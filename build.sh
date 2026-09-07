#!/bin/bash

PROJECT_PATH=$(pwd)
echo "当前路径： $PROJECT_PATH"

if [ -d "build" ];then
	echo " build 已经存在 "
else
	echo " build 不存在，创建build "
	mkdir build
fi

cd ./build
cmake ../
make -j16 

#mv $PROJECT_PATH/srsenb/enb.conf $PROJECT_PATH/srsenb/enb.conf.example
mv $PROJECT_PATH/srsenb/enb1.conf $PROJECT_PATH/srsenb/enb1.conf.example
mv $PROJECT_PATH/srsenb/enb2.conf $PROJECT_PATH/srsenb/enb2.conf.example
mv $PROJECT_PATH/srsenb/rb.conf $PROJECT_PATH/srsenb/rb.conf.example
mv $PROJECT_PATH/srsenb/rr.conf $PROJECT_PATH/srsenb/rr.conf.example
mv $PROJECT_PATH/srsenb/sib.conf $PROJECT_PATH/srsenb/sib.conf.example
mv $PROJECT_PATH/srsenb/sib_wx1.conf $PROJECT_PATH/srsenb/sib_wx1.conf.example
mv $PROJECT_PATH/srsenb/sib_wx2.conf $PROJECT_PATH/srsenb/sib_wx2.conf.example
#mv $PROJECT_PATH/srsenb/mib.conf $PROJECT_PATH/srsenb/mib.conf.example
mv $PROJECT_PATH/srsenb/mib1.conf $PROJECT_PATH/srsenb/mib1.conf.example
mv $PROJECT_PATH/srsenb/mib2.conf $PROJECT_PATH/srsenb/mib2.conf.example
mv $PROJECT_PATH/srsenb/recfg_wx.conf $PROJECT_PATH/srsenb/recfg_wx.conf.example


mv $PROJECT_PATH/srscnw/user_db.csv $PROJECT_PATH/srscnw/user_db.csv.example
mv $PROJECT_PATH/srsue/ue.conf $PROJECT_PATH/srsue/ue.conf.example
mv $PROJECT_PATH/mng/mng.conf $PROJECT_PATH/mng/mng.conf.example
sudo make install
sudo ldconfig

#mv $PROJECT_PATH/srsenb/enb.conf.example $PROJECT_PATH/srsenb/enb.conf
mv $PROJECT_PATH/srsenb/enb1.conf.example $PROJECT_PATH/srsenb/enb1.conf
mv $PROJECT_PATH/srsenb/enb2.conf.example $PROJECT_PATH/srsenb/enb2.conf
mv $PROJECT_PATH/srsenb/rb.conf.example $PROJECT_PATH/srsenb/rb.conf
mv $PROJECT_PATH/srsenb/rr.conf.example $PROJECT_PATH/srsenb/rr.conf
mv $PROJECT_PATH/srsenb/sib.conf.example $PROJECT_PATH/srsenb/sib.conf
mv $PROJECT_PATH/srsenb/sib_wx1.conf.example $PROJECT_PATH/srsenb/sib_wx1.conf
mv $PROJECT_PATH/srsenb/sib_wx2.conf.example $PROJECT_PATH/srsenb/sib_wx2.conf
#mv $PROJECT_PATH/srsenb/mib.conf.example $PROJECT_PATH/srsenb/mib.conf
mv $PROJECT_PATH/srsenb/mib1.conf.example $PROJECT_PATH/srsenb/mib1.conf
mv $PROJECT_PATH/srsenb/mib2.conf.example $PROJECT_PATH/srsenb/mib2.conf
mv $PROJECT_PATH/srsenb/recfg_wx.conf.example $PROJECT_PATH/srsenb/recfg_wx.conf

mv $PROJECT_PATH/srscnw/user_db.csv.example $PROJECT_PATH/srscnw/user_db.csv
mv $PROJECT_PATH/srsue/ue.conf.example $PROJECT_PATH/srsue/ue.conf

mv $PROJECT_PATH/mng/mng.conf.example $PROJECT_PATH/mng/mng.conf

#sudo cp ./srsenb/src/srsenb ../srsenb/ -a
#sudo cp ./srsue/src/srsue ../srsue/ -a

sudo chmod 777 srsran_install_configs.sh
sudo ./srsran_install_configs.sh service
#sudo ./srsran_install_configs.sh user

#sudo cp  $PROJECT_PATH/srsenb/*.conf /etc/srsran/ -a
#sudo cp  $PROJECT_PATH/srscnw/*.conf /etc/srsran/ -a
#sudo cp  $PROJECT_PATH/srscnw/*.csv /etc/srsran/ -a

#sudo cp  $PROJECT_PATH/srsenb/*.conf ~/.config/srsran/ -a
#sudo cp  $PROJECT_PATH/srscnw/*.conf ~/.config/srsran/ -a
#sudo cp  $PROJECT_PATH/srscnw/*.csv ~/.config/srsran/ -a

#sudo cp  $PROJECT_PATH/srsenb/*.conf /root/.config/srsran/ -a
#sudo cp  $PROJECT_PATH/srscnw/*.conf /root/.config/srsran/ -a
#sudo cp  $PROJECT_PATH/srscnw/*.csv /root/.config/srsran/ -a

#sudo cp  $PROJECT_PATH/srsue/*.conf /etc/srsran/ -a

sudo cp $PROJECT_PATH/dll/cur_libs/*  $PROJECT_PATH/build/ -a

if [ -d "/opt/AccessIot" ];then
        echo " /opt/AccessIot 已经存在 "
else
        echo " /opt/AccessIot 不存在，创建/opt/AccessIot "
	sudo mkdir /opt/AccessIot
fi

sudo cp $PROJECT_PATH/iqFile  /opt/AccessIot/ -a
chmod 777  $PROJECT_PATH/sh/*
cp $PROJECT_PATH/sh/*  $PROJECT_PATH/build/ -a

#sudo cp $PROJECT_PATH/srsenb/*.conf $PROJECT_PATH/build/ -a
#sudo cp $PROJECT_PATH/srscnw/*.conf $PROJECT_PATH/build/ -a
#sudo cp $PROJECT_PATH/srscnw/*.csv $PROJECT_PATH/build/ -a
#sudo cp $PROJECT_PATH/srsue/*.conf $PROJECT_PATH/build/ -a

sudo cp $PROJECT_PATH/mng/access $PROJECT_PATH/build/ -a
sudo cp $PROJECT_PATH/mng/IOT $PROJECT_PATH/build/ -a
sudo cp $PROJECT_PATH/mng/mng.conf $PROJECT_PATH/build/ -a
