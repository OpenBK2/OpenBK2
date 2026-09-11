#!/usr/bin/env python3
"""Regenerate Versions/Temporary/Engine/Sources/vendor/d3d9stub/src/Stubs.h.

The stub d3d9.dll lets the editor create a Direct3D 9 device where D3D9 will
not give it one -- the bk2probe desktop, an RDP session, a GPU-less CI runner
-- so that everything behind the viewport runs and can be tested. Nothing is
drawn.

Every COM interface the engine can reach gets a class here with a body for
every method, generated from the SDK's own d3d9.h so that the vtable order is
the header's and cannot be got wrong by hand:

  * IUnknown is real: QueryInterface answers for the interface and each of its
    bases, and AddRef/Release count references and delete at zero. A stub that
    got this wrong would leak or double-free long before anything interesting;
  * every other method records its name and arguments through D3D9_STUB and
    fails -- E_FAIL for an HRESULT, a zero of the right type otherwise.

The hand-written classes in the other files derive from these and override
what the engine actually calls. The trace is what says which that is: a line
marked "stub" is a method nothing has implemented yet.

The output is committed, like gen-granny-shim.py's, so the build does not need
Python and the header only changes when the SDK does.

Usage:
    scripts/port/gen-d3d9-stub.py
    scripts/port/gen-d3d9-stub.py --check     exit 1 if the file is stale
"""
import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..'))
OUT_PATH = os.path.join(ROOT, 'Versions', 'Temporary', 'Engine', 'Sources', 'vendor', 'd3d9stub',
                        'src', 'Stubs.h')
DXSDK = os.environ.get('DXSDK_ROOT', r'C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)')
HEADER = os.path.join(DXSDK, 'Include', 'd3d9.h')

# What gets a class. The four video and content protection interfaces at the
# end of d3d9.h are left out: nothing in this engine or in d3dx9 asks for them,
# and QueryInterface answers E_NOINTERFACE for anything not listed.
INTERFACES = [
    'IDirect3D9', 'IDirect3D9Ex',
    'IDirect3DDevice9', 'IDirect3DDevice9Ex',
    'IDirect3DStateBlock9', 'IDirect3DSwapChain9', 'IDirect3DSwapChain9Ex',
    'IDirect3DResource9', 'IDirect3DVertexDeclaration9', 'IDirect3DVertexShader9',
    'IDirect3DPixelShader9', 'IDirect3DBaseTexture9', 'IDirect3DTexture9',
    'IDirect3DVolumeTexture9', 'IDirect3DCubeTexture9', 'IDirect3DVertexBuffer9',
    'IDirect3DIndexBuffer9', 'IDirect3DSurface9', 'IDirect3DVolume9', 'IDirect3DQuery9',
]

IFACE_RE = re.compile(r'DECLARE_INTERFACE_\(\s*(\w+)\s*,\s*(\w+)\s*\)\s*\{(.*?)\n\};', re.S)
# STDMETHOD(Name)(THIS_ args) PURE;  or  STDMETHOD_(type,Name)(THIS) PURE;
METHOD_RE = re.compile(r'STDMETHOD(?:_\(\s*([^,]+?)\s*,\s*(\w+)\s*\)|\(\s*(\w+)\s*\))\s*\((.*?)\)\s*PURE\s*;', re.S)
IUNKNOWN = ('QueryInterface', 'AddRef', 'Release')


def parse():
    text = open(HEADER, encoding='latin-1').read()
    # D3D_DEBUG_INFO adds data members to the interfaces; nothing here defines
    # it, so they are dropped with the rest of the conditional.
    text = re.sub(r'#ifdef D3D_DEBUG_INFO.*?#endif', '', text, flags=re.S)
    interfaces = {}
    for m in IFACE_RE.finditer(text):
        name, base, body = m.groups()
        methods = []
        for mm in METHOD_RE.finditer(body):
            ret, name_, name_hr, params = mm.groups()
            methods.append((ret.strip() if ret else 'HRESULT', name_ or name_hr, params))
        interfaces[name] = (base, methods)
    return interfaces


def split_params(params):
    params = ' '.join(params.split())
    params = re.sub(r'^THIS_?\s*', '', params).strip()
    if not params:
        return []
    out = []
    for n, p in enumerate(params.split(',')):
        p = p.strip().replace('CONST ', 'const ')
        # A few are declared without a name -- MultiplyTransform, SetLight,
        # GetLight, GetFunction -- as a bare type or a type ending in '*'. The
        # trace wants a name, so they get their position.
        words = p.replace('*', ' * ').split()
        if p.endswith('*') or len([w for w in words if w != 'const']) == 1:
            out.append((p, 'arg%d' % n))
            continue
        name = re.search(r'(\w+)\s*$', p).group(1)
        type_ = p[:-len(name)].strip()
        out.append((type_, name))
    return out


def all_methods(interfaces, name):
    """Every method of the interface: its base's, in order, then its own.

    d3d9.h repeats the inherited methods inside each interface's braces, but
    not faithfully -- IDirect3D9Ex's copy leaves out RegisterSoftwareDevice,
    which it inherits all the same -- so the list is built from the base chain
    and the repeated copies are only used for what they add."""
    if name not in interfaces:
        return []
    base, own = interfaces[name]
    methods = list(all_methods(interfaces, base))
    names = {m[1] for m in methods}
    methods += [m for m in own if m[1] not in names]
    return methods


def chain(interfaces, name):
    names = []
    while name in interfaces:
        names.append(name)
        name = interfaces[name][0]
    names.append('IUnknown')
    return names


def declaring_interface(interfaces, iface, method):
    """The furthest base that already declares the method, for the trace."""
    owner = iface
    for base in chain(interfaces, iface)[1:]:
        if base in interfaces and any(m[1] == method for m in interfaces[base][1]):
            owner = base
    return owner


def render(interfaces):
    out = [
        '// Generated by scripts/port/gen-d3d9-stub.py from the DirectX SDK\'s d3d9.h.',
        '// Do not edit by hand: override in the hand-written classes instead.',
        '//',
        '// One class per interface, named C<interface without the I>Stub, with a real',
        '// IUnknown and every other method recorded and failed. See the generator for',
        '// why, and Trace.h for what D3D9_STUB records.',
        '',
        '#pragma once',
        '',
        '#include "Trace.h"',
        '',
        '#include <d3d9.h>',
        '',
        '#include <atomic>',
        '',
        'namespace ND3D9Stub',
        '{',
    ]
    for iface in INTERFACES:
        methods = all_methods(interfaces, iface)
        cls = 'C%sStub' % iface[1:]
        iids = ' || '.join('riid == IID_%s' % n for n in chain(interfaces, iface))
        out += [
            '',
            'class %s : public %s' % (cls, iface),
            '{',
            'public:',
            '\tvirtual ~%s() {}' % cls,
            '',
            '\t// IUnknown, for real.',
            '\tSTDMETHOD(QueryInterface)( REFIID riid, void **ppvObj ) override',
            '\t{',
            '\t\tif ( ppvObj == nullptr )',
            '\t\t{',
            '\t\t\treturn E_POINTER;',
            '\t\t}',
            '\t\tif ( %s )' % iids,
            '\t\t{',
            '\t\t\t*ppvObj = static_cast<%s *>( this );' % iface,
            '\t\t\tAddRef();',
            '\t\t\treturn S_OK;',
            '\t\t}',
            '\t\t*ppvObj = nullptr;',
            '\t\tD3D9_STUB( "%s", riid );' % iface,
            '\t\treturn E_NOINTERFACE;',
            '\t}',
            '\tSTDMETHOD_(ULONG, AddRef)() override',
            '\t{',
            '\t\treturn ++nRefs;',
            '\t}',
            '\tSTDMETHOD_(ULONG, Release)() override',
            '\t{',
            '\t\tconst ULONG nLeft = --nRefs;',
            '\t\tif ( nLeft == 0 )',
            '\t\t{',
            '\t\t\tdelete this;',
            '\t\t}',
            '\t\treturn nLeft;',
            '\t}',
        ]
        seen = set()
        for ret, name, params in methods:
            if name in IUNKNOWN or name in seen:
                continue
            seen.add(name)
            args = split_params(params)
            signature = ', '.join('%s %s' % a for a in args)
            owner = declaring_interface(interfaces, iface, name)
            macro_args = ', '.join(['"%s"' % owner] + [a[1] for a in args])
            if ret == 'HRESULT':
                decl = 'STDMETHOD(%s)( %s ) override' % (name, signature) if args else \
                       'STDMETHOD(%s)() override' % name
                result = 'return E_FAIL;'
            else:
                decl = 'STDMETHOD_(%s, %s)( %s ) override' % (ret, name, signature) if args else \
                       'STDMETHOD_(%s, %s)() override' % (ret, name)
                result = None if ret == 'void' else 'return {};'
            out += ['\t%s' % decl, '\t{',
                    '\t\t%s( %s );' % ('D3D9_STUB' if args else 'D3D9_STUB0', macro_args)]
            if result:
                out.append('\t\t%s' % result)
            out.append('\t}')
        out += [
            '',
            'protected:',
            '\tstd::atomic<ULONG> nRefs{ 1 };',
            '};',
        ]
    out += ['', '}  // namespace ND3D9Stub', '']
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--check', action='store_true')
    args = ap.parse_args()
    text = render(parse())
    if args.check:
        current = open(OUT_PATH, encoding='utf-8').read() if os.path.exists(OUT_PATH) else ''
        if current != text:
            print('%s is stale; rerun %s' % (OUT_PATH, os.path.basename(__file__)))
            return 1
        return 0
    os.makedirs(os.path.dirname(OUT_PATH), exist_ok=True)
    with open(OUT_PATH, 'w', newline='\n', encoding='utf-8') as f:
        f.write(text)
    print('wrote %s' % OUT_PATH)
    return 0


if __name__ == '__main__':
    sys.exit(main())
