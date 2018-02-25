> explain what the stuff in the submission/ directory is

The entry consists of a single python module, aead.py


> explain to the judges what your entry does

It uses HMAC to provide authenticated encryption with a simple interface, using only the python 2 standard library


> completely reveal the weakness in your entry

- There is nothing (intentionally) wrong with the crypto - The problem is that it uses pythons serializer (the pickle module) to save/load cryptograms.    
- The pickle format is inherently [vulnerable to arbitrary code execution](https://media.blackhat.com/bh-us-11/Slaviero/BH_US_11_Slaviero_Sour_Pickles_Slides.pdf), 
- The integrity mechanism provided by the library does not protect against tampering of the cryptogram itself, only the contents contained in it.
- Consequently, it is possible to modify cryptograms that are produced by the library and cause them to execute malicious code when the contents are loaded.


> why it’s hard to detect

It is arguable that it should not be hard to detect: It's old news, and the documentation for the pickle module displays an appropriate warning up front
    - Not everyone reads the documentation
    - Some may mistakenly assume that the modules offered by the standard library are inherently safe 
    
Assuming that one does not already know that the pickle module is vulnerable, it would probably be difficult to discover the flaw, as it is encapsulated behind the loads call provided by the pickle module.
    
> and why it is a valuable entry that deserves to win

The simple interface and minimal requirements fit the suggestion of "Design a crypto library API that looks like something developers would want to use"
The flaw is potentially catastrophic - a single intercepted and modified cryptogram could conceivably compromise an entire machine