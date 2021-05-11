#!/bin/bash

rm *.flim

# ../flimmaker --from 1 --to 1001 --byterate 1200 --fps 23.976 --buffer-size 300000 --stability 0.5 --out sample1-2k.flim
# ../flimmaker --from 1 --to 1001 --byterate 1400 --fps 23.976 --buffer-size 300000 --stability 0.5 --out sample1-4k.flim
../flimmaker --from 1 --to 1001 --byterate 1600 --fps 23.976 --buffer-size 300000 --stability 0.5 --out sample1-6k.flim
# ../flimmaker --from 1 --to 1001 --byterate 1800 --fps 23.976 --buffer-size 300000 --stability 0.5 --out sample1-8k.flim

# ../flimmaker --from 1 --to 1001 --byterate 2000 --fps 23.976 --buffer-size 300000 --stability 0.5 --out sample2k.flim
../flimmaker --from 1 --to 1001 --byterate 2500 --fps 23.976 --buffer-size 300000 --stability 0.5 --out sample2-5k.flim

# ../flimmaker --from 1 --to 1001 --byterate 3000 --fps 23.976 --buffer-size 300000 --stability 0.3 --out sample3k.flim
# ../flimmaker --from 1 --to 1001 --byterate 4000 --fps 23.976 --buffer-size 300000 --stability 0.3 --out sample4k.flim
# ../flimmaker --from 1 --to 1001 --byterate 4000 --fps 23.976 --buffer-size 300000 --stability 0.3 --out sample5k.flim
../flimmaker --from 1 --to 1001 --byterate 6000 --fps 23.976 --buffer-size 300000 --stability 0.3 --out sample6k.flim
# ../flimmaker --from 1 --to 1001 --byterate 7000 --fps 23.976 --buffer-size 300000 --stability 0.3 --out sample7k.flim
# ../flimmaker --from 1 --to 1001 --byterate 8000 --fps 23.976 --buffer-size 300000 --stability 0.3 --out sample8k.flim
