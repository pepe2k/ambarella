#!/bin/sh
case_interval=600
A_warp_C_unwarp(){
echo A_warp_C_unwarp
test_encode -i 4000x3000 -f 30 -V480p --hdmi --mixer 0 --enc-mode 10 -X --bsize 1920x1920 --bmaxsize 1920x1920 -Y --btype enc --bsize 720x720 --bmaxsize 720x720 --bunwarp 1 -J --btype off -K --bsize 1920x1920 --bmaxsize 1920x1920 --bunwarp 0 --btype prev -w 2048 -W 1920 -P --bsize 2816x2816 --bins 2816x2816 --bino 550x100 -A --smaxsize 720x720 --vout-swap 1

test_dewarp -M 0 -F 185 -R 1024 -a0 -s1920x1920 -m 1

test_encode -A -h720x720 -b1 -e
}

A_warp_C_warp(){
echo A_warp_C_warp
test_encode -i 4000x3000 -f 30 -V480p --hdmi --mixer 0 --enc-mode 10 -X --bsize 1920x1920 --bmaxsize 1920x1920 -Y --btype enc --bsize 720x720 --bmaxsize 720x720 --bunwarp 0 -J --btype off -K --bsize 1920x1920 --bmaxsize 1920x1920 --bunwarp 0 --btype prev -w 2048 -W 1920 -P --bsize 2816x2816 --bins 2816x2816 --bino 550x100 -A --smaxsize 720x720 --vout-swap 1

test_dewarp -M 0 -F 185 -R 1024 -a0 -s1920x1920 -m 1

test_encode -A -h720x720 -b1 -e
}

A_unwarp_C_unwarp(){
echo A_unwarp_C_unwarp
test_encode -i 4000x3000 -f 30 -V480p --hdmi --mixer 0 --enc-mode 10 -X --bsize 1920x1920 --bmaxsize 1920x1920 -Y --btype enc --bsize 720x720 --bmaxsize 720x720 --bunwarp 1 -J --btype off -K --bsize 1920x1920 --bmaxsize 1920x1920 --bunwarp 1 --btype prev -w 2048 -W 1920 -P --bsize 2816x2816 --bins 2816x2816 --bino 550x100 -A --smaxsize 720x720 --vout-swap 1

test_dewarp -M 0 -F 185 -R 1024 -a0 -s1920x1920 -m 1

test_encode -A -h720x720 -b1 -e
}


A_unwarp_C_off(){
echo A_unwarp_C_off
test_encode -i 4000x3000 -f 30 -V480p --hdmi --mixer 0 --enc-mode 10 -X --bsize 1920x1920 --bmaxsize 1920x1920 -Y --btype off -J --btype off -K --bsize 1920x1920 --bmaxsize 1920x1920 --bunwarp 1 --btype prev -w 2048 -W 1920 -P --bsize 2816x2816 --bins 2816x2816 --bino 550x100 -A --smaxsize 720x720 --vout-swap 1

test_dewarp -M 0 -F 185 -R 1024 -a0 -s1920x1920 -m 1
}


A_warp_C_off(){
echo A_warp_C_off
test_encode -i 4000x3000 -f 30 -V480p --hdmi --mixer 0 --enc-mode 10 -X --bsize 1920x1920 --bmaxsize 1920x1920 -Y --btype off -J --btype off -K --bsize 1920x1920 --bmaxsize 1920x1920 --bunwarp 0 --btype prev -w 2048 -W 1920 -P --bsize 2816x2816 --bins 2816x2816 --bino 550x100 -A --smaxsize 720x720 --vout-swap 1

test_dewarp -M 0 -F 185 -R 1024 -a0 -s1920x1920 -m 1
}

A_off_C_off(){
echo A_off_C_off
test_encode -i 4000x3000 -f 30 -V480p --hdmi --mixer 0 --enc-mode 10 -X --bsize 1920x1920 --bmaxsize 1920x1920 -Y --btype off -J --btype off -K --btype off -w 2048 -W 1920 -P --bsize 2816x2816 --bins 2816x2816 --bino 550x100 -A --smaxsize 720x720 --vout-swap 1

test_dewarp -M 0 -F 185 -R 1024 -a0 -s1920x1920 -m 1
}


A_off_C_unwarp(){
echo A_off_C_unwarp
test_encode -i 4000x3000 -f 30 -V480p --hdmi --mixer 0 --enc-mode 10 -X --bsize 1920x1920 --bmaxsize 1920x1920 -Y --btype enc --bsize 720x720 --bmaxsize 720x720 --bunwarp 1 -J --btype off -K --btype off -w 2048 -W 1920 -P --bsize 2816x2816 --bins 2816x2816 --bino 550x100 -A --smaxsize 720x720 --vout-swap 1

test_dewarp -M 0 -F 185 -R 1024 -a0 -s1920x1920 -m 1

test_encode -A -h720x720 -b1 -e
}


A_off_C_warp(){
echo A_off_C_warp
test_encode -i 4000x3000 -f 30 -V480p --hdmi --mixer 0 --enc-mode 10 -X --bsize 1920x1920 --bmaxsize 1920x1920 -Y --btype enc --bsize 720x720 --bmaxsize 720x720 --bunwarp 0 -J --btype off -K --btype off -w 2048 -W 1920 -P --bsize 2816x2816 --bins 2816x2816 --bino 550x100 -A --smaxsize 720x720 --vout-swap 1

test_dewarp -M 0 -F 185 -R 1024 -a0 -s1920x1920 -m 1

test_encode -A -h720x720 -b1 -e
}


if [ $# -lt 1 ];then
  while [ 1 ];do
    A_warp_C_unwarp
    sleep $case_interval
    A_warp_C_warp
    sleep $case_interval
    A_unwarp_C_unwarp
    sleep $case_interval
    A_unwarp_C_off
    sleep $case_interval
    A_warp_C_off
    sleep $case_interval
    A_off_C_off
    sleep $case_interval
    A_off_C_unwarp
    sleep $case_interval
    A_off_C_warp
    sleep $case_interval
  done
fi


if [ $# -eq 1 ];then
  case "$1" in
  A_warp_C_unwarp)
    A_warp_C_unwarp
    ;;

  A_warp_C_warp)
    A_warp_C_warp
    ;;

  A_unwarp_C_unwarp)
    A_unwarp_C_unwarp
    ;;

  A_unwarp_C_off)
    A_unwarp_C_off
    ;;

  A_warp_C_off)
    A_warp_C_off
    ;;

  A_off_C_off)
    A_off_C_off
    ;;

  A_off_C_unwarp)
    A_off_C_unwarp
    ;;

  A_off_C_warp)
    A_off_C_warp
    ;;

  *)
    echo "No such case !"
    ;;
  esac
fi
