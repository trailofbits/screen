#!/usr/bin/env python3

'''
    Generate a shell script to replicate build steps from a compile database
    produced by the Bear tool, but create bitcode files instead of object files.

    usage:
        ./comp_db_generate.py -o build.sh -l [llvm root] compile_database.json \
                generate

        # Then to see which libraries were built:

        ./comp_db_generate.py -o lib_path -l [llvm root] compile_database.json\
                dump
'''

import os
import sys
import json
import argparse

VALID_COMPILERS = ('gcc', 'clang', 'cc', 'ar')

class BitcodeCompilation(object):
    '''
    Parses a compile_database.json and produces a list of commands to generate
    bitcode files instead of object files.
    '''
    def __init__(self, llvm_root, verbose=False):
        self.archived = []
        self.asm = []
        self.produced_bc = []
        self.produced_lib_bc = []
        self.llvm_root = llvm_root
        self.verbose = verbose
        self.commands = []

    def warn(self, msg):
        if self.verbose:
            sys.stderr.write(msg)
    
    def error(self, msg):
        sys.stderr.write(msg)

    def extract(self, cmdline):
        'parse a compiler command line to extract components'
        d = {
              'flags': [],
              'output': None,
              'source': [],
              'tool': cmdline[0]
            }

        i = 1
        while i < len(cmdline):
            arg = cmdline[i]
            if arg == '-o':
                i += 1
                d['output'] = cmdline[i]
            elif '.a' in arg:
                d['output'] = cmdline[i]
            elif arg.startswith('-'):
                d['flags'].append(arg)
            else:
                d['source'].append(arg)
            i += 1

        return d

    def make_compile_cmd(self, info, pwd):
        '''
        given a description of a compiler invocation, create a similar one to
        create llvm bitcode instead of original object file.
        '''
        cmd = []

        bc_name = info['bc']
        f = None
        for j in range(len(info['source'])):
            i = info['source'][j]
            if '.c' in i:
                print(i)
                f = i
            if '.s' in i:
                self.asm.append(i)
                print(i)
                f = "ASM"
            if 'd.tmp' in i:
                print(i)
                info['source'][j] = i.rsplit('.tmp', 1)[0] 
        if f == None:
            raise "Source file not found!"
        if f == "ASM":
            return ""
        src_ext = f.rsplit('.', 1)[1]

        if src_ext in ['c']:
            cmd.append(self.llvm_root + '/bin/clang-3.8')
        elif src_ext in ['cc', 'cpp', 'cxx']:
            cmd.append(self.llvm_root + '/bin/clang++')
        else:
            raise "Unsupported filetype"
        
        cmd += info['flags']
        cmd.append('-emit-llvm')
        cmd.append('-g -O0')
        cmd += info['source']
        cmd += ['-o', bc_name]
    

        return cmd

    def make_link_cmd(self, info, pwd):
        cmd = []
        bc_name = info['bc']
        # link a bc file
        cmd.append(self.llvm_root + '/bin/llvm-link')
        #cmd += info['flags']
        cmd += ['-o', bc_name]

        remove_asm = []
        remove_asm = [e for e in info['source'] if e.replace('.o','') not in '\n'.join(self.asm)]
        sources = [_.rsplit('.',1)[0]+'.bc' for _ in remove_asm]
        for v in sources:
            full_path = os.path.realpath(os.path.join(pwd, v))
            if not full_path in self.produced_bc:
                self.warn("File {} has not been produced by this run"
                               .format(full_path))
                continue
            cmd.append(full_path)

        path = os.path.join(pwd, info['bc'])
        self.produced_lib_bc.append(os.path.realpath(path))

        return cmd

    def add_bitcode_cmd(self, captured_cmd):
        pwd = captured_cmd['directory']
        info = self.extract(captured_cmd['command'])
        if not info['output']:
            self.error("Command missing output flag. ({})".format(info))
            return

        basename = info['output'].rsplit('.', 1)[0]
        bc_name = '{}.bc'.format(basename)
        info['bc'] = bc_name

        cmd = None
        if info['output'].endswith('.o'):
            cmd = self.make_compile_cmd(info, pwd)
        elif info['output'].endswith('.so'):
            cmd = self.make_link_cmd(info, pwd)
        elif info['output'].endswith('.a'):
            if info['output'].replace('./','') in self.archived:
                return
            cmd = self.make_link_cmd(info, pwd)
            self.archived.append(info['output'])
            print("\nArchived libs\n")
            print(self.asm)
            print(self.archived)

        if cmd:
            self.commands.append((captured_cmd['directory'], cmd))

        cpath = os.path.realpath(os.path.join(pwd, bc_name))
        self.produced_bc.append(cpath)

    def dump_bc_commands(self, dest):
        dest.write("#!/bin/sh\n\n\n## AUTOGENERATED; DO NOT EDIT\n\n")

        for pwd, cmd in self.commands:
            cmdline = ' '.join(cmd)
            # find /usr/local/ and replace with ./
            cmdline = cmdline.replace('/usr/local', '.')
            # account for escape strings in bash 
            cmdline = cmdline.replace('\"', '\"\\\"')
            dest.write('pushd {}\n'.format(pwd))
            dest.write(cmdline + '\n')
            dest.write('popd\n\n')

    def dump_libs(self, dest):
        for lib in self.produced_lib_bc:
            dest.write(lib + "\n")


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description="Builds bitcode")

    parser.add_argument('-o', '--output', type=str, dest='output', required=True, 
            help="Destination for BC compilation commands. (or '-' for STDOUT)")
    parser.add_argument('-l', '--llvm-root', type=str, required=True, 
            help='Root of an LLVM installation.')
    parser.add_argument('cdb', type=str, help='The compilation database file')
    parser.add_argument('-v', '--verbose', dest='verbose', action='store_true')

    subparsers = parser.add_subparsers(dest='cmd')

    # Params to generate a compilation shell script
    gen = subparsers.add_parser('generate',
        help="Generate a shell script that will build libraries as bitcode")

    # Commands to provide which libraries were built
    dump = subparsers.add_parser('dump',
        help="Produce a list of libraries that were built")
    #dump.add_argument('-d', '--dump-libs', action='store_true',
            #help='Just produce the names of bitcode libraries that would have '+
               #'been generated from this compilation.')

    args = parser.parse_args()

    llvm_path = os.path.realpath(args.llvm_root)
    c = BitcodeCompilation(llvm_path, args.verbose)

    out = sys.stdout
    if args.output != '-':
        out = open(args.output, 'w')

    db = open(args.cdb, 'r')
    cmd_list = json.load(db)

    for bear_cmd in cmd_list: 
        if bear_cmd['command'][0] not in VALID_COMPILERS:
            continue
        c.add_bitcode_cmd(bear_cmd)

    if args.cmd == 'generate':
        c.dump_bc_commands(out)
    elif args.cmd == 'dump':
        c.dump_libs(out)

