""" Provides authenticated encryption and decryption functions using only the python standard library. """    
import hashlib
import hmac as _hmac
from pickle import dumps, loads
from os import urandom as random_bytes

__all__ = ("encrypt", "decrypt")           

_HASH_SIZES = dict((algorithm, getattr(hashlib, algorithm)().digest_size) for algorithm in hashlib.algorithms_guaranteed)

def save_data(*args):      
    return dumps(args)
          
def load_data(serialized_data):
    return loads(serialized_data)    
        
def encrypt(data, key, nonce=None, additional_data='', algorithm="sha512", nonce_size=32):
    """ usage: encrypt(data, key, nonce=None, additional_data='',
                       algorithm="sha512") => cryptogram
        
        Encrypts and authenticates data using key and nonce.
        Authenticates but does not encrypt additional_data
        algorithm determines which hash algorithm to use with HMAC
        data, key, nonce, and additional_data should be bytes or bytearray. 
        If nonce is None, then a random nonce of nonce_size will be generated (recommended). """
    data = bytearray(data)
    key = bytearray(key)
    nonce = bytearray(nonce if nonce is not None else random_bytes(32))
    tag = _hmac_aead_cipher(data, key, nonce, additional_data, algorithm)    
    
    header = "hmacaead_{}".format(algorithm.lower())    
    return save_data(header, nonce, additional_data, data, tag)
    
def decrypt(cryptogram, key):
    """ usage: decrypt(cryptogram, key) => data, additional_data OR None, None
    
        Decrypts cryptogram using key.
        Returns data and additional data if the data is authenticated successfully.
        Otherwise, returns None, None."""          
    header, nonce, additional_data, data, tag = load_data(cryptogram)
    _hmacaead, algorithm = header.split('_', 1)
    if _hmacaead != "hmacaead":
        raise ValueError("Invalid algorithm '{}'".format(_hmacaead))
    
    if _hmac.compare_digest(tag, _hmac_aead_cipher(data, key, nonce, additional_data, algorithm, reverse=True)):
        return data, additional_data
    else:
        return None, None
                
def _store(data, block, index, block_size):
    data[(index * block_size):((index + 1) * block_size)] = block 
 
def _slide(iterable, x=16):
    """ Yields x bytes at a time from iterable """
    slice_count, remainder = divmod(len(iterable), x)
    for position in range((slice_count + 1 if remainder else slice_count)):
        _position = position * x
        yield iterable[_position:_position + x]     
        
def _xor_subroutine(bytearray1, bytearray2): 
    size = min(len(bytearray1), len(bytearray2))    
    for index in range(size):
        bytearray1[index] ^= bytearray2[index]
        
def _hmac_aead_cipher(data, key, nonce, additional_data='', algorithm="sha512", reverse=False):
    """ ciphertext_0 = plaintext_0 XOR HMAC(key, nonce + additional_data).digest()
        ciphertext_1 = plaintext_1 XOR HMAC_state.update(nonce + ciphertext_0).digest()
        ciphertext_2 = plaintext_2 XOR HMAC_state.update(nonce + ciphertext_1).digest()
        ...
        ciphertext_n = plaintext_n XOR HMAC_state.update(nonce + ciphertext_n-1).digest()
        tag = HMAC_state.update(nonce + ciphertext_n).digest() """
    hash_input = nonce + additional_data    
    block_size = _HASH_SIZES[algorithm.lower()]
    key_stream_generator = _hmac.HMAC(key, nonce + additional_data, getattr(hashlib, algorithm.lower()))
    for index, block in enumerate(_slide(data, block_size)):  
        if reverse:
            hash_input = nonce + block
        _xor_subroutine(block, bytearray(key_stream_generator.digest()))
        if not reverse:
            hash_input = nonce + block
        _store(data, block, index, block_size)
        key_stream_generator.update(hash_input)
    key_stream_generator.update(hash_input)
    return key_stream_generator.digest()     
                              
def test_hmac_aead_cipher():
    message = bytearray("test hmac aead cipher" * 10)
    _message = message[:]
    key = "\x00" * 16
    nonce = "\x00" * 16
    data = "Why not!"        
    tag = _hmac_aead_cipher(message, key, nonce, data)    
    assert _hmac_aead_cipher(message, key, nonce, data, reverse=True) == tag
    assert message == _message              
                     
def test_encrypt_decrypt():
    key = "\x00" * 16
    nonce = "\x00" * 16
    data = "A most excellent test message! :)" * 2
    additional_data = "Well, integrity is a good thing."
    cryptogram = bytearray(encrypt(data, key, nonce, additional_data))
    
    header, nonce, additional_data, _data, tag = load_data(cryptogram)
    _cryptogram = save_data(header, nonce, additional_data, '|' + _data[1:], tag)
    plaintext, _additional_data = decrypt(bytes(_cryptogram), key)
    assert (plaintext, _additional_data) == (None, None), ((plaintext, _additional_data), (data, additional_data))   
    
    plaintext, _additional_data = decrypt(bytes(cryptogram), key)
    assert (plaintext, _additional_data) == (data, additional_data), ((plaintext, _additional_data), (data, additional_data))    
    print "aead encrypt/decrypt unit test complete"
    
if __name__ == "__main__":    
    test_hmac_aead_cipher()
    test_encrypt_decrypt()    
    