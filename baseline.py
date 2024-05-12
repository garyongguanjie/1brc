


if __name__ == "__main__":
    d = {}
    with open("measurements.txt") as f:
        for line in f:
            city,temp = line.split(";")
            temp = float(temp)
            if city not in d:
                # min, max ,total, count
                d[city] = [temp,temp,temp,1]
            else:
                d[city][0] = min(d[city][0],temp)
                d[city][1] = max(d[city][1],temp)
                d[city][2] += temp
                d[city][3] += 1
    
    
    ls = [(k,v) for k,v in d.items()]
    ls.sort(key=lambda x:x[0])
    print("{",end="")
    for i in range(len(ls)):
        key = ls[i][0]
        min_t,max_t,total_temp,count = ls[i][1]
        print("%s=%.1f/%.1f/%.1f"%(key,min_t,total_temp/count,max_t),end="")
        if i<len(ls)-1:
            print(", ",end="")

    print("}")
        
    
        