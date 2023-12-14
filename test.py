import time
fo=open("test.txt","a")
s=str(time.time())+"\n"
fo.write(s)
fo.write(s)
fo.close()
