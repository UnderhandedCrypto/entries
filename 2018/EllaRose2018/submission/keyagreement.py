import ast

from utilities import random_integer

SECURITY_LEVEL = 32

def load_parameters():   
    """ usage: load_parameters() => parameters : dict
    
        Returns a dictionary of parameters which includes a base point, modulus, and parameter sizes.
        The dictionary is read from parameters.txt. """
    try:
        with open("parameters.txt", 'r') as _file:
            parameters = ast.literal_eval(_file.read())
    except (IOError, SyntaxError):            
        print("Parameters not yet generated; Run 'python backdoor.py --gen' before using keyagreement.py")
        raise SystemExit()        
    return parameters
 
PARAMETERS = load_parameters()
 
def generate_private_key(parameters=PARAMETERS):
    """ usage: generate_private_key(parameters=PARAMETERS) => private_key : int
        
        Returns a private key for use with the key agreement scheme. 
        It is recommended to use generate_keypair instead. """
    return random_integer(parameters["s_size"]) 

def generate_public_key(private_key, parameters=PARAMETERS):
    """ usage: generate_public_key(private_key, parameters=PARAMETERS) => public_key : int
        
        Returns a pubic key for use with the key agreement scheme.
        It is recommended to use generate_keypair instead. """
    s = private_key 
    r = random_integer(parameters["r_size"]) 
    return ((parameters['e'] * s) + r) % parameters['n']
    
def generate_keypair(parameters=PARAMETERS):
    """ usage: generate_keypair(parameters=PARAMETERS) => public_key : int, private_key : int
        
        Returns a public key private key pair for use with the key agreement scheme. """
    private_key = generate_private_key(parameters)
    public_key = generate_public_key(private_key, parameters)
    return public_key, private_key
    
def key_agreement(public_key, private_key, parameters=PARAMETERS):
    """ usage: key_agreement(public_key, private_key, parameters=PARAMETERS) => shared_secret
        
        Returns a shared secret. """
    return ((public_key * private_key) % parameters['n']) >> parameters["shift"]

def unit_test():
    from unittesting import test_key_agreement
    test_key_agreement("key agreement", generate_keypair, key_agreement, iterations=10000)  
    
if __name__ == "__main__":
    unit_test()
    