# rfm69_F007TH
receiver for the Ambient wheater sensor F007TH with the RFM69 / sx1231.
uses the OOK option, needs the 433 MHz version of the module
no interrupt. (but you might use DIO0)
There might be room for improvement at the receiver configuration part.
e.g. rxbw bandwidth was increased to cover all my seonsors
right now it is more a proof of concept, and yes it works.
ToDo, add crc check, 
current consumption might be improved, but so far no need.
