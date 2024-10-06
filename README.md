# ProjetoFinal

This project addresses the challenges faced in universalizing communication between
IoT (Internet of Things) devices for smart homes from different manufacturers and from
distinct ecosystems, as well as the high energy consumption associated with the use of the
Wi-Fi protocol in these environments.
With the appearance of home automation software, these problems have escalated, and
protocols such as Zigbee, BLE, and Z-Wave begun to be used to integrate and control IoT
devices. However, these protocols have not solved the interoperability issues between
devices from different manufacturers, and although some have lower energy consumption
than Wi-Fi, they present other problems.
Given these challenges and the lack of effective solutions, two protocols have been
developed recently: Matter and Thread. The Matter protocol aims to universalize
communication between IoT devices from different manufacturers and improve
interoperability between different ecosystems, while the Thread protocol is a communication
protocol specifically created for smart home environments, distinguished by its energy
efficiency and lack of single points of failure (there is no loss of communication with devices
in the network).
Initially, we conducted a detailed study on both protocols to acquire knowledge and
create the necessary documentation for the development of our scenario. This research
allowed the development of various practical scenarios. First, we integrated and controlled
an LED, and secondly, a device consisting of two sockets and two switches (implementing
the solutions first in Matter over Wi-Fi and then in Matter over Thread).
For the implementation of the proposed scenario of the device with two sockets and two
switches, the following components were used: an ESP32-C6, two sockets, two switches,
two relays, a mini PC, a Sonoff USB ZBDongle-E adapter, and a Mikrotik router. The Sonoff
USB adapter was responsible for the entire Thread network, the Mikrotik router managed
the Wi-Fi network, and the mini PC hosted the Home Assistant platform to enable the
integration and control of the solution. Finally, the ESP32-C6 acted as the controller of the
prototype, directly interfacing with the sockets and switches.
viii
Throughout the tests conducted and the results obtained, we found that despite facing
many challenges due to the recent introduction of the protocols, we successfully overcame
them and achieved a functional prototype with the various protocols. We identified and
presented solutions to all the problems encountered in developing the scenarios in Matter
over Wi-Fi and Matter over Thread, making this process simpler and more intuitive. We
took a significant step forward in the evolution of Smart Homes and in developing new
solutions based on these protocols.
