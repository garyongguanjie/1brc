const fs = require('node:fs');
const readline = require('node:readline');

async function processLineByLine() {
    d = {};
    const fileStream = fs.createReadStream('measurements.txt');

    const rl = readline.createInterface({
        input: fileStream,
        crlfDelay: Infinity,
    });
    // Note: we use the crlfDelay option to recognize all instances of CR LF
    // ('\r\n') in input.txt as a single line break.

    for await (const line of rl) {
        // Each line in input.txt will be successively available here as `line`.
        let [city, temp] = line.split(";");
        temp = Number(temp);
        if (city in d) {
            d[city][0] = Math.min(d[city][0], temp);
            d[city][1] = Math.max(d[city][1], temp);
            d[city][2] += temp;
            d[city][3] += 1;
        } else {
            d[city] = [temp, temp, temp, 1];
        }
    }
    let arr = [];
    for (const key of Object.keys(d)) {
        arr.push(key);
    }
    arr.sort();

    process.stdout.write('{')
    for (let i=0;i<arr.length;i++){
        let [min_t,max_t,total_temp,count]  = d[arr[i]];
        process.stdout.write(`${arr[i]}:${min_t.toFixed(1)}/${(total_temp/count).toFixed(1)}/${max_t.toFixed(1)}`)
        if (i<arr.length-1){
            process.stdout.write(', ');
        }
        
    }
    console.log('}');

}
processLineByLine();