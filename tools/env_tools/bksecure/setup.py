import os

from setuptools import setup, find_packages

# Read the long description from README if available
long_description = ''
if os.path.exists('README.md'):
    with open('README.md', encoding='utf-8') as f:
        long_description = f.read()

setup(
    name='beken_utils',
    version='0.1.0',
    author='beken',
    author_email='your@email.com',
    description='beken_utils tools.',
    long_description=long_description,
    long_description_content_type='text/markdown',
    url='https://gitlab.bekencorp.com/armino/customer/signify/bk_tools',
    packages=find_packages(exclude=["tests*", "examples*", "build*", "dist*"]),
    include_package_data=True,
    package_data={
    'tools': ['*', '*/*', '*/*/*', '*/*/*/*'], 
    },
    # If main.py and version.py are top-level modules, include them:
    py_modules=['main', 'version'],
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
            # This will install a `beken_utils` command that invokes the CLI in main.py
            'beken_utils=main:cli',
        ],
    },
)
