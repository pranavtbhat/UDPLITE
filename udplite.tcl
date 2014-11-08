#set constants
set error [lindex $argv 0]
set time [lindex $argv 1]
set pkt_size [lindex $argv 2]
set mode [lindex $argv 3]
set ratio [lindex $argv 4]
set options [lindex $argv 5]

#If constants weren't provided use defaults
if {$error == ""} {
	set error 0.0001

	puts ""
	puts "Usage : ns udplite.tcl <error_rate> <time_of_simulation> <packet_size> <mode>"
	puts ""
	puts "Using default error of 0.0001"
} 

if {$time == ""} {
	set time 10.0
	puts "Using default duration of simulation as 10.0s"
}

if {$pkt_size == ""} {
	set pkt_size 240
	puts "Using default packet size for audio : 240 bytes"
}

if {$mode == ""} {
	set mode 0
	puts "Using UDPLite mode: 0"
}

if {$ratio == ""} {
	set ratio 10
	puts "Using default ratio between data segments and checksum segments to be 10"
}

if {$options == ""} {
	set options "All"
}

#Create a simulator object
set ns [new Simulator]

#Open the NAM trace file

set tf [open out.tr w]
$ns trace-all $tf

#set nf [open udplite_out.nam w]
#$ns namtrace-all $nf

proc finish {} {
        global ns tf em udplite sink options error
        $ns flush-trace
        #close $nf
        close $tf
        #exec nam udplite_out.nam &
        

        set num_recv [$sink set pkts_recv_]
        set num_sent [$udplite set pkts_sent_]
        set tp [expr "(double($num_recv)/$num_sent)*100"]

        if {$options == "All"} {
        	puts ""
        	puts "Number of packets sent by source   : $num_sent"
        	puts "Number of packets received at sink : $num_recv"

        	puts "Observed throughput was            : $tp %"
        } else {
        	puts "$error $tp"
        }
        exit 0
}

set n0 [$ns node]
set n1 [$ns node]

$ns duplex-link $n0 $n1 2.5Mb 10ms DropTail
$ns duplex-link-op $n0 $n1 orient right

$ns duplex-link-op  $n0 $n1 queuePos 0.5 
$ns queue-limit $n0 $n1 1000

set udplite [new Agent/UDPLite]
$udplite set packetSize_ $pkt_size
$udplite set udp_mode_ $mode
$udplite set ratio_ $ratio
$ns attach-agent $n0 $udplite

set sink [new Agent/UDPLite]
$sink set udp_mode_ $mode
$sink drop-target [new Agent/Null]
$sink set ratio_ $ratio
$ns attach-agent $n1 $sink

$ns connect $udplite $sink

set cbr [new Application/Traffic/CBR]
$cbr set packetSize_ $pkt_size
$cbr attach-agent $udplite

set em [new ErModel]
$em set rate_ $error
$em drop-target [new Agent/Null]

$ns link-lossmodel $em $n0 $n1

$ns at 0.0 "$cbr start"

$ns at [expr "$time-1.0"] "$cbr stop"
$ns at $time "finish"

$ns run


