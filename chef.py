#chef can turn a .obj file from blender into a c source file

def main():
    import sys
    
    with open(sys.argv[1], "r") as file:
        lines = file.readlines()
        name = [line for line in lines if line.startswith("o ")][0].split(' ')[1].strip()
        section_v = [line for line in lines if line.startswith("v ")]
        section_vt = [line for line in lines if line.startswith("vt ")]
        section_f = [line for line in lines if line.startswith("f ")]

        v = [line.strip('v\n').split(' ') for line in section_v]
        vt = [line.strip('v\n').split(' ') for line in section_vt]
        f = [line.strip('v\n').split(' ') for line in section_f]

        #float name[] = {
            # the lines comma separated
        #};

        with open(name.lower() + ".c", "w+") as output:
            output.writelines("#pragma once\n\n")

            output.writelines(f"float {name.lower()}_vtx[] = {'{'}\n")
            [output.writelines("\t" + "f,\t".join(vertex[1:]) + "f,\n") for vertex in v]
            output.writelines({'};\n'})

            output.writelines(f"\nfloat {name.lower()}_tx[] = {'{'}\n")
            [output.writelines("\t" + ",\t".join(texcoord[1:]) + ",\n") for texcoord in vt]
            output.writelines({'};\n'})

            output.writelines(f"\nint {name.lower()}_idx[] = {'{'}")
            [output.writelines(tx.split("/")[0] + ", ") for idx in f for tx in idx[1:]]
            output.writelines({'};'})

            output.writelines(f"\nint {name.lower()}_tx_idx[] = {'{'}")
            [output.writelines(tx.split("/")[1] + ", ") for idx in f for tx in idx[1:]]
            output.writelines({'};'})

            output.writelines(f"\n#define {name.lower()}_vtxcnt {len(v)}")
            output.writelines(f"\n#define {name.lower()}_idxcnt {len([idx for idx in f for tx in idx[1:]])}")
            output.writelines(f"\n#define {name.lower()}_tx_idxcnt {len([idx for idx in f for tx in idx[1:]])}")



if __name__ == "__main__":
    main()