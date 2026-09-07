./set-cpu.sh
sudo ./mng/src/xwmng --config_file=../mng/mng.conf --mng_files.enb_conf_path=$(pwd) | tee mnglog.txt
