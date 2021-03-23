# Modes

The protocol document stipulates the details of packets that can be send in setup versus normal mode. 
In contrast, this document describes on a functional level what you might expect.

## Setup

When the Crownstone arrives from the factory it has its **factory settings**. It is ready to be put to use by its owner. 
There are no backdoors, no default username and password combinations, no region restrictions, etc.
This means you will have to **configure** the Crownstone to be properly owned by you. This process is called the **setup** process.

In the Crownstone app you will see that a Crownstone is in the **setup mode** when you press the "+" token to add one. 
After this, the app will guide you through a series of steps that will add the proper keys to the Crownstone so all subsequent communication with it is secure.

### Settings

The Crownstone has its relay turned off when it's getting out of the factory. There's no current going through from input to output. 
Hence, if you let an installer install the Crownstones you'll have to either (1) get them out of setup mode or (2) manually turn them on while they are in 
setup mode. The latter is not an option available to the standard Crownstone app (contact us if you need this as e.g. **real estate developer** delivering homes).

The reason for this is the following. Although it is very easy to wire the Crownstone, it is always possible that people make mistakes. 
For example, mixing inputs and outputs. The supply that the Crownstone uses internally is wired from the input. If the relays would be on, the input and the output
is connected. Hence, the Crownstone gets still its supply. Now, the first time you would switch the Crownstone off, you would cut its supply and be scratching 
your head big time! We don't want people to run into such difficult problems. If the installation is done incorrectly, there should be immediate feedback and the 
Crownstone should not appear in the app or (perhaps in the future) indicate that it has been wired incorrectly.

### Behaviour

In setup mode, the Crownstone can still be switched. There is also a subset of behaviour that's still active. For example, protection measures.

### Factory reset

When performing a **factory reset** a Crownstone goes back in **setup mode**. After this all information on the Crownstone is removed. 
You can then use this Crownstone in a different sphere or give it to friends or families and they can add it as if it's a new Crownstone.
