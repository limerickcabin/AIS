python aispost.py &
while :
do
	date >> ais.txt
	sudo rtl_ais -R on -n
done
