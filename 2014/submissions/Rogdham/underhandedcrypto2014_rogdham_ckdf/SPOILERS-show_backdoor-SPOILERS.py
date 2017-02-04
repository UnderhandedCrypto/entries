#!/usr/bin/env python2

'''
Show backdoor script for CKDF.

Copyheart Rogdham, 2014 ~ Sharing is caring... Please copy!
See http://copyheart.org/ for more details.

Licence of this file: CC0

    This program is free software. It comes without any warranty, to the extent
    permitted by applicable law. You can redistribute it and/or modify it under
    the terms of the CC0 1.0 Universal Licence (French).
    See https://creativecommons.org/publicdomain/zero/1.0/deed.fr for more
    details.

Tested with Python 2.7.
'''


#           ____  ____   ___ ___ _     _____ ____
#          / ___||  _ \ / _ \_ _| |   | ____|  _ \
#          \___ \| |_) | | | | || |   |  _| | |_) |
#           ___) |  __/| |_| | || |___| |___|  _ <
#          |____/|_|    \___/___|_____|_____|_| \_\
#
#                 _    _     _____ ____ _____
#                / \  | |   | ____|  _ \_   _|
#               / _ \ | |   |  _| | |_) || |
#              / ___ \| |___| |___|  _ < | |
#             /_/   \_\_____|_____|_| \_\|_|
#
# This script reveals the backdoor in CKDF.
# Please make sure you are OK with this before reading it!
#


from ckdf import *


def center(msg, length):
    '''Center the msg with spaces to match the length'''
    l = length - len(msg)
    assert l >= 0
    return ' ' * (l / 2) + msg + ' ' * ((l + 1) / 2)


passwords = ('p@ssw0rd', 'qwerty', 'y8P!*8AH', 'monkey', 'letmein', 'anything')
users = ('alice', 'bob', 'admin', 'eve')
kdfs = (ckdf(user, password) for user, password in zip(users, passwords))


if __name__ == '__main__':
    import sys
    print
    print 'SPOILER ALERT: this will reveal the backdoor!!!'
    if raw_input('Type YES if sure: ').strip() != 'YES':
        print 'Ok, not showing anything here.'
        sys.exit(0)
    # Just attempt to login with every password against every user kdf
    # and show the results in a table...
    print
    print center('CREDENTIALS', 17), '||',
    print center('LOGIN ATTEMPT WITH PASSWORD', 63)
    print ' User  | Password || ' + ' | '.join(center(p, 8)
                                               for p in passwords)
    print '-------+----------++-' + '-+-'.join('-' * 8
                                               for _ in passwords) + '-'
    for user, password, kdf in zip(users, passwords, kdfs):
        print '', center(user, 5), '|', center(password, 8), '||',
        print ' | '.join(center('PASS' if ckdf_check(user, p, kdf) else '', 8)
                         for p in passwords)
