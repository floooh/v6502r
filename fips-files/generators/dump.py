#-------------------------------------------------------------------------------
#   dump.py
#   Dump binary files into C arrays.
#-------------------------------------------------------------------------------

Version = 7

import sys
import os.path
import yaml
import genutil

#-------------------------------------------------------------------------------
def get_file_path(filename, src_dir, file_path) :
    '''
    Returns absolute path to an input file, given file name and 
    another full file path in the same directory.
    '''
    return '{}/{}{}'.format(os.path.dirname(file_path), src_dir, filename)

#-------------------------------------------------------------------------------
def get_file_cname(filename) :
    return 'dump_{}'.format(filename).replace('.','_')

#-------------------------------------------------------------------------------
def gen_header(out_hdr, src_dir, files, as_text) :
    with open(out_hdr, 'w') as f:
        f.write('#pragma once\n')
        f.write('// #version:{}#\n'.format(Version))
        f.write('// machine generated, do not edit!\n')
        items = {}
        for file in files :
            file_path = get_file_path(file, src_dir, out_hdr)
            if os.path.isfile(file_path) :
                with open(file_path, 'rb') as src_file:
                    file_data = src_file.read()
                    file_name = get_file_cname(file)
                    file_size = os.path.getsize(file_path)
                    items[file_name] = file_size
                    if as_text:
                        f.write('char {}[{}] = {{\n'.format(file_name, file_size+1))
                    else:
                        f.write('unsigned char {}[{}] = {{\n'.format(file_name, file_size))
                    num = 0
                    for byte in file_data :
                        if sys.version_info[0] >= 3:
                            f.write(hex(ord(chr(byte))) + ', ')
                        else:
                            f.write(hex(ord(byte)) + ', ')
                        num += 1
                        if 0 == num%16:
                            f.write('\n')
                    if as_text:
                        f.write('0\n};\n')
                    else:
                        f.write('\n};\n')
            else :
                genutil.fmtError("Input file not found: '{}'".format(file_path))

#-------------------------------------------------------------------------------
def generate(input, out_src, out_hdr) :
    with open(input, 'r') as f :
        desc = yaml.load(f)
    if 'src_dir' in desc:
        src_dir = desc['src_dir'] + '/'
    else:
        src_dir = ''
    files = desc['files']
    input_files = [input]
    for file in files:
        input_files.append(get_file_path(file, src_dir, out_hdr))
    if genutil.isDirty(Version, input_files, [out_hdr]) :
        as_text = False
        if 'as_text' in desc:
            as_text = desc['as_text']
        gen_header(out_hdr, src_dir, files, as_text)
