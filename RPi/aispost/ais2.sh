python aispostsimple.py &
while :
do
	cd ../rtl-ais
 	date >> ais.txt
	sudo ./rtl_ais -R on -n
	sleep 1
done
