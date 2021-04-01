import base64
import array
pub_1 = 'wsPGv9bw2Q0mLNR8Q+TA5q4e6GQzRIiPxl5gXtrsZi8='
pub_1_arr=list(base64.b64decode(pub_1))
pub_1_arr_rev=[hex(a) for a in pub_1_arr]
print(pub_1_arr_rev)
pub_1 = 'IAB2ptLioKUQA+T4SH8AAAAAAAAAAAAAEAPk+Eh/AEA= '
pub_1_arr=list(base64.b64decode(pub_1))
pub_1_arr_rev=[hex(a) for a in pub_1_arr]
print(pub_1_arr_rev)
pub_2 = 'l9OSbVr/VD2XhyCbVdvaRoIjhWxCuW1iWaQ0GeHkLzo='
pub_2_arr=list(base64.b64decode(pub_2))
pub_2_arr_rev=[hex(a) for a in pub_2_arr]
print(pub_2_arr_rev)
sec2 = 'OBgpK9a1/XWQruL4SH8AAAAAAAAAAAAAAMfgNUl/AEA='
sec2_arr=list(base64.b64decode(sec2))
sec2_arr_rev=[hex(a) for a in sec2_arr]
print(sec2_arr_rev)

pet_et = 'KqO9fF5bvHtJFh6uWSDBnaO4JZu6hi/AJTjLbSyPklE='
pet_et_arr=list(base64.b64decode(pet_et))
pet_et_arr_rev=[hex(a) for a in pet_et_arr]
print(pet_et_arr_rev)

pet_rt = 'iscm1Ih0+xfKL38bF/jONgeGkhqSKaWyaokgxGiT+1U='
pet_rt_arr=list(base64.b64decode(pet_rt))
pet_rt_arr_rev=[hex(a) for a in pet_rt_arr]
print(pet_rt_arr_rev)
