#!/usr/bin/env python
import argparse
import sys


def bin2header(data, var_name='var', define='header'):
    out = []
    if var_name == "index":
        var_name = 'index_p'
    out.append('#ifndef __' + define.upper() + '_H__\n' + '#define __' +
               define.upper() + '_H__\n' + 'const char {var_name}[] = {{'.format(var_name=var_name))
    l = [data[i:i+12] for i in range(0, len(data), 12)]
    for i, x in enumerate(l):
        line = ', '.join(['0x{val:02x}'.format(val=ord(c)) for c in x])
        out.append('  {line}{end_comma}'.format(
            line=line, end_comma=',' if i < len(l)-1 else ''))
    out.append('};')
    out.append('const int {var_name}_len = {data_len};\n'.format(
        var_name=var_name, data_len=len(data)))
    return '\n'.join(out)


def main():
    parser = argparse.ArgumentParser(
        description='Generate binary header output')
    parser.add_argument('-i', '--input', required=True, help='Input file')
    parser.add_argument('-d', '--define', required=True, help='Define name')
    parser.add_argument('-o', '--out', required=False, help='Output file')
    parser.add_argument('-v', '--var', required=True,
                        help='Variable name to use in file')

    args = parser.parse_args()
    if not args:
        return 1

    with open(args.input, 'r') as f:
        data = f.read()

    out = bin2header(data, args.var, args.define) + '\n#endif\n'

    if args.out:
        with open(args.out, 'w') as f:
            f.write(out)
    else:
        print(out)

    return 0


if __name__ == '__main__':
    sys.exit(main())
