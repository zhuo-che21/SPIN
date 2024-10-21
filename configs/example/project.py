import os, argparse, sys

os.system("sed -i '$a\ Experiment begind' network_stats_project.txt")
os.system("sed -i '$a\ For 3D Mesh:' network_stats_project.txt")
for alg in ['3', '2']:
    if alg == '3':
        a = 'adaptive'
    else:
        a = 'deterministic'
    os.system("sed -i '$a\ algorithm is %s' network_stats_project.txt" % (a))
    for i in ['uniform_random','tornado','bit_complement','bit_reverse','bit_rotation','neighbor','shuffle','transpose',]:
        os.system("sed -i '$a\ i=%s' network_stats_project.txt" % (i))
        for inj in [0.01,0.03,0.05,0.07,0.09,0.11,0.13, 0.16,0.19,0.21, 0.24,0.26,0.28,0.3,0.33,0.35,0.37,0.4,0.42, 0.44,0.46, 0.48,0.5,0.52, 0.54,0.55, 0.57,0.6,0.62,0.64, 0.66, 0.68, 0.7,0.72, 0.74, 0.76, 0.78, 0.8,0.82, 0.84, 0.86, 0.88, 0.9, 0.92, 0.94, 0.96, 0.98,1]:
            os.system("./build/NULL/gem5.opt \
            configs/example/garnet_synth_traffic.py \
            --network=garnet --num-cpus=64 --num-dirs=64  --topology=Mesh_XYZ --routing-algorithm=%s --mesh-rows=4 --mesh-heights=4 --inj-vnet=0 --synthetic=%s --sim-cycles=10000 --injectionrate=%s" % (alg, i, inj))
            os.system("sed -i '$a\ inj=%s' network_stats_project.txt" % (inj))
            os.system("grep 'packets_injected::total' m5out/stats.txt | sed 's/system.ruby.network.packets_injected::total\s*/packets_injected = /' >> network_stats_project.txt")
            os.system("grep 'packets_received::total' m5out/stats.txt | sed 's/system.ruby.network.packets_received::total\s*/packets_received = /' >> network_stats_project.txt")
            os.system("grep 'average_packet_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_latency\s*/average_packet_latency = /' >> network_stats_project.txt")
            os.system("grep 'average_hops' m5out/stats.txt | sed 's/system.ruby.network.average_hops\s*/average_hops = /' >> network_stats_project.txt")
            os.system("grep 'average_packet_queueing_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_queueing_latency\s*/average_packet_queueing_latency = /' >> network_stats_project.txt")
            os.system("grep 'average_packet_network_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_network_latency\s*/average_packet_network_latency = /' >> network_stats_project.txt")
            os.system("grep 'success_rate' m5out/stats.txt | sed 's/system.ruby.network.success_rate\s*/success_rate = /' >> network_stats_project.txt")        

os.system("sed -i '$a\ For 2D Mesh:' network_stats_project.txt")
for i in ['uniform_random','tornado','bit_complement','bit_reverse','bit_rotation','neighbor','shuffle','transpose',]:
    os.system("sed -i '$a\ i=%s' network_stats_project.txt" % (i))
    for inj in [0.01,0.03,0.05,0.07,0.09,0.11,0.13, 0.16,0.19,0.21, 0.24,0.26,0.28,0.3,0.33,0.35,0.37,0.4,0.42, 0.44,0.46, 0.48,0.5,0.52, 0.54,0.55, 0.57,0.6,0.62,0.64, 0.66, 0.68, 0.7,0.72, 0.74, 0.76, 0.78, 0.8,0.82, 0.84, 0.86, 0.88, 0.9, 0.92, 0.94, 0.96, 0.98,1]:
        os.system("./build/NULL/gem5.opt \
        configs/example/garnet_synth_traffic.py \
        --network=garnet --num-cpus=64 --num-dirs=64  --topology=Mesh_XY --mesh-rows=8 --inj-vnet=0 --synthetic=%s --sim-cycles=10000 --injectionrate=%s" % (i, inj))
        os.system("sed -i '$a\ inj=%s' network_stats_project.txt" % (inj))
        os.system("grep 'packets_injected::total' m5out/stats.txt | sed 's/system.ruby.network.packets_injected::total\s*/packets_injected = /' >> network_stats_project.txt")
        os.system("grep 'packets_received::total' m5out/stats.txt | sed 's/system.ruby.network.packets_received::total\s*/packets_received = /' >> network_stats_project.txt")
        os.system("grep 'average_packet_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_latency\s*/average_packet_latency = /' >> network_stats_project.txt")
        os.system("grep 'average_hops' m5out/stats.txt | sed 's/system.ruby.network.average_hops\s*/average_hops = /' >> network_stats_project.txt")
        os.system("grep 'average_packet_queueing_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_queueing_latency\s*/average_packet_queueing_latency = /' >> network_stats_project.txt")
        os.system("grep 'average_packet_network_latency' m5out/stats.txt | sed 's/system.ruby.network.average_packet_network_latency\s*/average_packet_network_latency = /' >> network_stats_project.txt")
        os.system("grep 'success_rate' m5out/stats.txt | sed 's/system.ruby.network.success_rate\s*/success_rate = /' >> network_stats_project.txt")        
            