import os

from setuptools import setup

BASE_DIR = os.path.abspath(os.path.dirname(__file__))
SOURCE_ROOT = os.path.join(BASE_DIR, 'bksecure')

README_MD = os.path.join(SOURCE_ROOT, 'README.md')
VERSION_PY = os.path.join(SOURCE_ROOT, 'version.py')
TOOLS_DIR = os.path.join(SOURCE_ROOT, 'tools')

def read_version(version_file):
    version = '1.0.0.6'
    if not os.path.exists(version_file):
        return version
    with open(version_file, 'r', encoding='utf-8') as f:
        for line in f:
            line = line.strip()
            if line.startswith('VERSION') and '=' in line:
                value = line.split('=', 1)[1].strip().strip("'").strip('"')
                if value:
                    version = value
                break
    return version

long_description = ''
if os.path.exists(README_MD):
    with open(README_MD, encoding='utf-8') as f:
        long_description = f.read()

tool_files = []
for root, _, files in os.walk(TOOLS_DIR):
    for file_name in files:
        abs_path = os.path.join(root, file_name)
        rel_path = os.path.relpath(abs_path, TOOLS_DIR)
        tool_files.append(rel_path)

setup(
    name='bksecure',
    version=read_version(VERSION_PY),
    author='beken',
    author_email='',
    license="MIT",
    description='bksecure tools.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://gitlab.bekencorp.com/armino/customer/signify/bk_tools',
    packages=['bksecure', 'bksecure.scripts', 'bksecure.tools'],
    package_dir={
        'bksecure': 'bksecure',
        'bksecure.scripts': 'bksecure/scripts',
        'bksecure.tools': 'bksecure/tools',
    },
    include_package_data=False,
    package_data={
        'bksecure.tools': tool_files,
    },
    install_requires=[
        'cbor==1.0.0',
        'cbor2==5.6.4',
        'click==7.1.2',
        'click_option_group==0.5.6',
        'devicetree==0.0.2',
        'intelhex==2.3.0',
        'pycryptodome==3.17',
        'setuptools==52.0.0',
        'cryptography==43.0.0',
    ],
    classifiers=[
        'Programming Language :: Python :: 3',
        'Operating System :: OS Independent',
    ],
    python_requires='>=3.6',
    entry_points={
        'console_scripts': [
            'bksecure=bksecure.main:cli',
        ],
    },
)
