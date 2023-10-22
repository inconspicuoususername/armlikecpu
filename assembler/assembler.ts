import fs from 'fs'
import readline from 'readline'
const events = require('events');

const enum operands
{
    A = 0, //op1, op2, op3
    B = 1, //op, op2
    C = 2,//op1
    D //none
}


interface Opcode
{
    opstyle: operands
    imm: boolean
    forceimm?: number
    bits: number
}

const opcodes: {[key: string] : Opcode} =
{
    'add': {
        opstyle: operands.A,
        imm: true,
        bits: 0b10000
    },
    'sub': {
        opstyle: operands.A,
        imm: true,
        bits: 0b10001
    },
    'mul': {
        opstyle: operands.A,
        imm: true,
        bits: 0b10010
    },
    'and': {
        opstyle: operands.A,
        imm: true,
        bits: 0b10011
    },
    'or': {
        opstyle: operands.A,
        imm: true,
        bits: 0b10100
    },
    'xor': {
        opstyle: operands.A,
        imm: true,
        bits: 0b10101
    },
    'lsr': {
        opstyle: operands.A,
        imm: true,
        bits: 0b100110
    },
    'asr': {
        opstyle: operands.A,
        imm: true,
        bits: 0b10111
    },
    'asl': {
        opstyle: operands.A,
        imm: true,
        bits: 0b11000
    },
    'mov': {
        opstyle: operands.B,
        imm: true,
        bits: 0b01000
    },
    'ld': {
        opstyle: operands.B,
        imm: true,
        bits: 0b01001
    },
    'st': {
        opstyle: operands.B,
        imm: true,
        bits: 0b01001
    },
    'call': {
        opstyle: operands.B,
        imm: true,
        bits: 0b11001
    },
    'ret': {
        opstyle: operands.C,
        imm: true,
        bits: 0b11010
    },
    'br': {
        opstyle: operands.B,
        imm: false,
        forceimm: 1,
        bits: 0b00001
    },
    'jmp': {
        opstyle: operands.B,
        imm: true,
        bits: 0b00010
    },
    'push': {
        opstyle: operands.C,
        imm: false,
        bits: 0b00001
    },
    'pop': {
        opstyle: operands.C,
        imm: false,
        bits: 0b00001
    },
    'int': {
        opstyle: operands.C,
        imm: true,
        forceimm: 1,
        bits: 0b11100
    }
    
}



function dec2bin(dec: number) {
    return (dec >>> 0).toString(2);
  }


const registers: {[key:string]: number} = {
    "r0": 0,
    "r1": 1,
    "r2": 2,
    "r3": 3,
    "r4": 4,
    "r5": 5,
    "r6": 6,
    "r7": 7
}

let binarystr: string = ""

function validateOperands(op: string[])
{
    let binstr: string = "";
    if(op.length === 0)
        return true;
    let ins = opcodes[op[0]!];

    if(ins === undefined)
        throw "Invalid op"

    let opstyle : operands = ins.opstyle;
    let opcstr = dec2bin(ins.bits);
    binstr += opcstr.padStart(5-opcstr.length + opcstr.length, '0');

    switch(opstyle)
    {
        case operands.A:
            binstr += validregister(op[1]!)
            binstr += validregister(op[2]!)
            binstr += validimm2(op[3]!, ins);
            break;
        case operands.B:
            binstr += validregister(op[1]!)
            binstr += validimm2(op[2]!, ins);
            break;
        case operands.C:
            binstr += validimm2(op[1]!, ins);
            break;
    }
    binarystr += binstr;

    return true;
}

function validimm2(op: string, ins: Opcode)
{
    let out : string = "";

    const lastregpad = () => {
        switch(ins.opstyle)
        {
            case(operands.A):
                out+= '00'
                break;
            case(operands.B):
            {
                out+= '00000'
                break;
            }
            case(operands.B):
            {
                out+= '00000000'
                break;
            }
        }
    }
    if(ins.imm)
    {
        if(ins.forceimm)
            out = validimm(op, ins);
        else
        {
            try {
                out = validimm(op, ins);
            }
            catch {
                out = validregister(op);
                lastregpad();
            }
        }
    }
    else
    {
        out = validregister(op)
        lastregpad();
    }

    return out;
}
function validimm(imm: string, op: Opcode)
{
    let out: string = ""
    if(imm[0] !== '#')
        throw "Invalid immediate"
    
    imm = imm.substring(1);

    let immn = dec2bin(parseInt(imm));

    let padl: number = 0;
    let padlforce: number = 0;

    switch(op.opstyle)
    {
        case operands.A:
            padl = 4;
            padlforce = 5;
            break;
        case operands.B:
            padl = 7;
            padlforce = 8;
            break;
        case operands.C:
            padl = 10;
            padlforce = 11;
            break;

    }

    if(op.forceimm)
    {
        immn = immn.substring(0, padlforce);
        immn = immn.padStart(padlforce-immn.length + immn.length, '0')
        out += immn;
    }
    else if (op.imm)
    {
        immn = immn.substring(0, padl);
        immn = immn.padStart(padl-immn.length + immn.length, '0')
        out += immn;
        out += '1'; //imm enable
    }

    return out;

}
function validregister(str:string)
{
    const res = registers[str]

    if(res !== undefined)
    {
        let out: string = dec2bin(res);
        let padl = 3-out.length + out.length;
        out = out.padStart(padl, '0')
        
        return out;
    }
    else
        throw "invalid reg";
}

const symbols: string[] = []

async function parse()
{
    const lines: string[] = []

    const rl = readline.createInterface({
        input: fs.createReadStream("assembly.txt"),
        crlfDelay: Infinity
      });
  
    rl.on('line', (line: string) => {
        lines.push(line);
    });
    await events.once(rl, 'close');
    let linectr = 0;

    for(let line of lines)
    {
        line = line.replace(/\s{1,}/, ",")
        line = line.replace(/\s{1,}/g, "")
        let tokens: string[] = line.split(",");

        if (tokens.length > 2 && tokens.length < 4)
        {
            if(tokens.length == 1) {
                if(tokens[0]![tokens.length-1] == ':')
                    symbols.push(tokens[0]!.substring(0, -1))
                else
                {
                    throw new Error("Invalid token");
                }
            }           
        }

        for(let i in tokens)
        {
            tokens[i] = tokens[i]!.toLowerCase();
        }

        try {
            validateOperands(tokens);
        }
        catch(err)
        {
            console.log("Err line " + linectr + ": " + err)
            process.exit(1);
        }
        

        linectr++;
    }

    const hexarr: string[] = []
    let binstring4 = "";
    let ctr = 0;


    for(const char of binarystr)
    {
        binstring4 += char;
        ctr++;
        if(ctr === 4)
        {
            hexarr.push(parseInt(binstring4,2).toString(16));
            binstring4 = "";
            ctr = 0;
        }
    }

    fs.writeFile('assembly_OUT.txt', hexarr.join().replaceAll(",", ""), err => {
        if (err) {
          console.error(err);
        }
        // file written successfully
      });

    console.log(hexarr.join());
}


parse();