python aispostsimple.py &
while :
do
	date >> ais.txt
	sudo rtl_ais -R on
	sleep 1
done
