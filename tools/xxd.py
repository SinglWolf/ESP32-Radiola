#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import argparse
import sys
import io

def bytes_to_c_arr(data):
    return [format(b, '#04x') for b in data]

def bin2header(data, var_name='var', define='header'):
    out = []
    out.append('#ifndef __' + define.upper() + '_H__\n' + '#define __' +
               define.upper() + '_H__\n' + 'const char {var_name}[] = {{'.format(var_name=var_name))
    l = [data[i:i+16] for i in range(0, len(data), 16)]
    for i, x in enumerate(l):
        line = ', '.join(bytes_to_c_arr(x))
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

    with open(args.input, 'rb') as f:
        data = f.read()
    out = bin2header(data, args.var, args.define) + '\n#endif\n'

    if args.out:
        with io.open(args.out, 'w', encoding='utf-8') as f:
            f.write(out)
    else:
        print(out)

    return 0


if __name__ == '__main__':
    sys.exit(main())
