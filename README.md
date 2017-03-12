# tunerd
An OpenBSD (radioio) program "webapp" that switches radio stations on a (controllable) radio tuner card

OpenBSD has a radio driver layer that supports several FM radio tuner cards.
With a compatible card, this program
- acts as an HTTP server (purpose specific, not general)
- displays to browser/clients the current radio station frequency
- a NEXT button in browser sends HTTP POST to change radio frequency (to next in a list of station "presets")
- on station change, server sends an update to display new frequency to all listening browsers via ServerSentEvents (SSE) / EventSource


Intended environment (or, what I created it for):
For instance, a house with a multiroom(multifloor) in-ceiling/wall speakers with a central audio amplifier with stereo line-level inputs, but no intrinsic radio tuner.
With a small (low-power) computer with a compatible radio tuner card, connecting the computer line-out to the central amplifier inputs, this progam allows anyone on the local area network (say, a different floor in the house) to see the current radio station, and if desired change to the next radio station in a list of preset station frequencies.
(In my case, the central amplifier is in the first floor family room, and is connected to in-ceiling speakers in my 2nd floor bedroom, with a wall panel controller for on/off and volume. Now, from my bedroom I can browse on my wifi tablet to connect to the computer/tuner and change the radio station. All the devices that have browsers open on the root page will immediately show the updated radio frequency.)



Requirements:
- OpenBSD system
- compatible FM tuner card. See http://man.openbsd.org/radio.4 for information on chipsets. Tested with Hauppauge WinTV PCI model 61381, which has a Brooktree 878 chip. Found on eBay for about $20
- root privileges


Instructions:
hardware:
- install the compatible radio card
- connect FM antenna to FM coax
- connect tuner card line-out to motherboard line-in (blue)
- connect motherboard line-out (green) to external amplifier
- confirm radio card is working

probably a good idea to configure the OpenBSD system for a fixed/static IP address
or else have a DNS entry for that computer


program installation
- get source code archive

- create directory /var/tunerd/
`mkdir /var/tunerd`

- configure/customize for your location:
edit a desired list of presets
frequency is in kHz, so 95.7 MHz is stored as 95700, one number per line
`vi presets.txt`
copy your configured presets.txt to directory
`cp presets.txt /var/tunerd`

- copy root.html to directory
`cp root.html /var/tunerd`


customize source if you want:
default listen port is 80 - edit main.c, function init()
(if you do not have root privileges, you may need to change to a port above the restricted range 1-1024)
default listen both IPv4 and IPv6 - can edit main.c, function init()


- make executable
`make`

- move executable to directory
`mv tunerd /usr/local/sbin/`

- configure friendly to rc system
`cp etc.rd_tunerd /etc/rc.d/tunerd`

- manually start
`/etc/rc.d/tunerd start`

Check log for errors
Log is at /var/tunerd/tunerd.log

open a browser to that computer:
e.g. http://192.168.1.128/



Troubleshooting
Radio tuner card and OpenBSD radio/mixer troubleshooting

`radioctl`
(displays current radio settings)

make sure radio is not muted
if 'mute=on'
change mute setting by:
`radioctl mute=off`

set a radio frequency to one known to provide good signal at your location
change frequency by, for example:
`radioctl frequency=95.7`



Things that are neat about this progam (possibly only to me)

Beyond learning event-driven programming with poll(), and TCP/IP socket management


Server Sent Events
I could not figure out how to write a true "push" ServerSentEvents CGI program.
Perhaps it was my lack of knowledge, but with PHP CGI I could not figure out a way
to truly send Server Sent Events to a set of listeners only when a POST event arrived at the webserver

To me, it seemed that that was what the Server Sent Events standard was intended to do.

I wrote my own framework for a webserver that would keep listener sockets open, track them (in a list/array)
and send to all of them, on an event-driven basis.

I think the framework is correct, since the browsers all seem to respond as I would expect

