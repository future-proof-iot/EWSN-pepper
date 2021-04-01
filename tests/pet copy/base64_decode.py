import base64
import array

def hexdump(prefix, arr):
    hex_arr = [hex(a) for a in arr]
    print(prefix, '{',', '.join(hex_arr),'}')

pub_1 = 'wsPGv9bw2Q0mLNR8Q+TA5q4e6GQzRIiPxl5gXtrsZi8='
hexdump('pub_1 =', list(base64.b64decode(pub_1)))

sec_1 = 'IAB2ptLioKUQA+T4SH8AAAAAAAAAAAAAEAPk+Eh/AEA= '
hexdump('sec_1 =', list(base64.b64decode(sec_1)))

pub_2 = 'l9OSbVr/VD2XhyCbVdvaRoIjhWxCuW1iWaQ0GeHkLzo='
hexdump('pub_2 =', list(base64.b64decode(pub_2)))

sec2 = 'OBgpK9a1/XWQruL4SH8AAAAAAAAAAAAAAMfgNUl/AEA='
hexdump('sec2 =', list(base64.b64decode(sec2)))

pet_et = 'KqO9fF5bvHtJFh6uWSDBnaO4JZu6hi/AJTjLbSyPklE='
hexdump('pet_et =', list(base64.b64decode(pet_et)))

pet_rt = 'iscm1Ih0+xfKL38bF/jONgeGkhqSKaWyaokgxGiT+1U='
hexdump('pet_rt =', list(base64.b64decode(pet_rt)))
