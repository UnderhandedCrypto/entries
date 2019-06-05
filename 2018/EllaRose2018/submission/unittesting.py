from timeit import default_timer

from utilities import random_integer, size_in_bits

class UnitTestFailure(BaseException): pass
        
def determine_key_size(key):    
    sizes = []
    try:
        sizes.append(size_in_bits(key))
    except TypeError:        
        for item in key:
            try:
                for _item in item:                    
                    sizes.append(size_in_bits(_item))
            except TypeError:                    
                try:
                    sizes.append(size_in_bits(item))    
                except TypeError:
                    for _item in item:
                        for __item in _item:
                            sizes.append(size_in_bits(__item))
    return sizes     
    
def test_key_agreement_time(iterations, key_agreement, generate_keypair, key_size=32):        
    if iterations == 0:
        return None    
    print("Agreeing upon {} {}-byte keys...".format(iterations, key_size))                
    before = default_timer()
    for count in range(iterations):                     
        public_key, private_key = generate_keypair()
        key = key_agreement(public_key, private_key)
    after = default_timer()
    print("Time required: {}".format(after - before))   
    
def test_key_agreement(algorithm_name, generate_keypair, key_agreement, 
                       iterations=1024, key_size=32):
    print("Beginning {} unit test...".format(algorithm_name))

    print("Validating correctness...")    
    for count in range(iterations):
        public_key, private_key = generate_keypair()
        public_key2, private_key2 = generate_keypair()
        key = key_agreement(public_key2, private_key)
        _key = key_agreement(public_key, private_key2)
        if key != _key:
            raise UnitTestFailure("test_key_agreement failed after {} tests".format(count))
    print("...done")
                    
    public_sizes = determine_key_size(public_key)
    private_sizes = determine_key_size(private_key)
    print("Public key size : {}".format(sum(public_sizes)))
    print("Private key size: {}".format(sum(private_sizes)))
    print("Shared Key size : {}".format(sum(determine_key_size(key))))
    print("(sizes are in bits)")
    print("{} unit test passed".format(algorithm_name))
    