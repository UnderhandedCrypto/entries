import ast
import pprint

from utilities import random_integer, modular_inverse

SECURITY_LEVEL = 32
                        
def generate_parameter_sizes(security_level=SECURITY_LEVEL):
    """ usage: generate_parameter_sizes(security_level=SECURITY_LEVEL) => parameters : dict
    
        Returns a dictionary of parameter sizes. """
    s_size = security_level
    parameters = {"d_size" : (security_level * 2) + 1, "s_size" : s_size,
                  "n_size" : security_level * 8}
                  
    n_size = parameters["n_size"]        
    parameters["k_size"] = n_size / 2    
        
    # key agreement parameters
    modulus_size = security_level * 8    
    parameters.update({"r_size" : modulus_size - (s_size * 2) - 2,
                       "shift" : (modulus_size - security_level) * 8})                  
    return parameters

PARAMETERS = generate_parameter_sizes(SECURITY_LEVEL)
                                              
def generate_backdoor_private_key(parameters=PARAMETERS):
    """ usage: generate_backdoor_private_key(parameters=PARAMETERS) => backdoor_key : tuple
    
        Generates a private backdoor key. 
        It is recommended to use generate_backdoor_keypair instead. """
    d_size = parameters["d_size"]
    n_size = parameters["n_size"]
    k = random_integer(parameters["k_size"])     
    while True:
        d = random_integer(d_size) | (1 << (d_size * 8)) 
        n = random_integer(n_size) | (1 << (n_size * 8))             
        try:
            modular_inverse(d, n - k)
        except ValueError:
            continue
        else:
            break            
    return d, n, n - k
    
def generate_backdoor_public_key(private_key, parameters=PARAMETERS):
    """ usage: generate_backdoor_public_key(private_key, parameters=PARAMETERS) => public_key : tuple
    
        Generates a public key for use as parameters in the key agreement scheme. 
        It is recommended to use backdoor_generate_keypair instead. """
    d, n, n_k = private_key
    e = modular_inverse(d, n_k)    
    return e, n
    
def generate_backdoor_keypair(parameters=PARAMETERS):
    """ usage: generate_backdoor_keypair(parameters=PARAMETERS) => public_key : tuple, private_key : tuple
    
        Generates a public_key private_key pair. 
        public_key is for use as parameters in the key agreement scheme. """
    private_key = generate_backdoor_private_key(parameters)
    public_key = generate_backdoor_public_key(private_key, parameters)
    return public_key, private_key
    
def recover_key(ka_public_key, backdoor_private_key):
    """ usage: recover_key(ka_public_key, backdoor_private_key) => ka_private_key : int
    
        Recovers the private key from the public key (where both keys are from the key agreement scheme).
        The recovered private key can then be used to participate in the key agreement scheme. """        
    d, n, n_k = backdoor_private_key
    return ((ka_public_key * d) % (n_k)) % d

def generate_backdoored_parameters(parameters=PARAMETERS):    
    """ usage: generate_backdoored_parameters(parameters=PARAMETERS) => None
        
        Generates backdoored parameters for the key agreement scheme and writes the result to disk.
        Two txt files are generated:
            - parameters.txt contains a dict of parameters for the key agreement scheme to use
            - backdoor.txt contains the backdoor key for compromising the key agreement scheme
        
        The key agreement parameters can be loaded via keyagreement.load_parameters.
        The backdoor key can be loaded via the load_backdoor_key function. """
    backdoor_public_key, backdoor_key = generate_backdoor_keypair(parameters)
    e, n = backdoor_public_key
    parameters = parameters.copy()
    parameters['e'] = e
    parameters["n"] = n
    del parameters["d_size"]
    del parameters["k_size"]
    parameters_string = pprint.pformat(parameters)
    with open("parameters.txt", 'w') as _file:
        _file.write(parameters_string + '\n')
        _file.flush()
        
    d, n, private_modulus = backdoor_key
    with open("backdoor.txt", 'w') as _file:
        _file.write(pprint.pformat(backdoor_key) + '\n')
        _file.flush()    
    
def load_backdoor_key():
    """ usage: load_backdoor_key() => backdoor_key : tuple
        
        Returns the backdoor key from the file backdoor.txt created by generate_backdoored_parameters. """
    try:
        with open("backdoor.txt", 'r') as _file:
            backdoor_key = ast.literal_eval(_file.read())        
    except (IOError, SyntaxError):
        print("Parameters not yet generated; Run 'python backdoor.py --gen' before using keyagreement.py")
        raise SystemExit()        
    return backdoor_key
    
def unit_test(parameters=PARAMETERS):                
    from keyagreement import generate_keypair, key_agreement
    
    backdoor_key = load_backdoor_key()    
    for count in range(10000):
        pub1, priv1 = generate_keypair()
        pub2, priv2 = generate_keypair()
        share = key_agreement(pub2, priv1)
        
        backdoor_priv1 = recover_key(pub1, backdoor_key)
        if backdoor_priv1 != priv1:
            raise Warning("Unit test failed after {} successfully compromised key agreements".format(count))
        backdoor_share = key_agreement(pub2, backdoor_priv1)
        if backdoor_share != share:
            raise Warning("Unit test failed after {} successfully compromised key agreements".format(count))           

def test_vulnerability_to_weiners_attack():   
    from keyagreement import PARAMETERS as parameters
    from math import log
    backdoor_key = load_backdoor_key()
    n = parameters["n"]
    threshold = log((2 ** (log(n, 2) / 4)) / 3, 2)
    d = backdoor_key[0]
    d_size = log(d, 2)
    if d_size < threshold:
        raise Warning("private key vulnerable to Weiner's attack")
        
if __name__ == "__main__":
    import sys
    try:
        gen_command = sys.argv[1]
    except IndexError:
        test_vulnerability_to_weiners_attack()
        print("Beginning unit test...")
        unit_test()
        print("...done")
    else:
        if gen_command == "--gen":
            print("Generating backdoored parameters...")
            generate_backdoored_parameters()
            print("...done")
        else:
            print("Unrecognized command {}".format(gen_command))
        