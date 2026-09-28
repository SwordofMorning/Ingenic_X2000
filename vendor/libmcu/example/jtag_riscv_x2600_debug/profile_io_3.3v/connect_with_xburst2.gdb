set $cnt=5
set $entry_point=(unsigned int)&_start

set $ccu_to_host = 0x12a0000c
set $reset_pc_value = 0x12a00004

set {int}$reset_pc_value = 0
set {int}$ccu_to_host = $entry_point | 0x80000000


while ($cnt)
    print "riscv reset pc value is: "
    print *(int*)$reset_pc_value
    if (*(int*)$ccu_to_host == 0 && *(int*)$reset_pc_value == $entry_point)
        set $cnt = 0
    else
        set $cnt=$cnt-1
        shell sleep 1
        if ($cnt==0)
            print "wait xburst2 ack timeout\n"
            quit -1
        end
    end
end