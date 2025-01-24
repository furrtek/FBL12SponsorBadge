# FBL12SponsorBadge

Super sponsor convention badge for the 12th edition of FBL

* Custom segment display
* Microphone and speaker
* Non-volatile audio storage
* Touch-based digital synth
* Bidirectional IR communication
* Embedded LiPo battery with USB-C charger

# Tech stuff

Any weird design choices were made for cost optimization (i.e. part was the cheapest at the time)

* Can be assembled by hand, smallest part is 0603
* Full BOM except screws can be purchased from LCSC
* PCBs all 2-layers except backboard (no copper at all)
* Fw written in plain C with ST libraries (HAL only used for USB and touch sensing, everything else is LL)
* Custom USB bootloader to update MCU and ext flash with UF2 files

# IR frame format

TODO
