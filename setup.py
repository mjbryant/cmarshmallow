#!/usr/bin/env python
# -*- coding: utf-8 -*-
from setuptools import Extension, setup, find_packages


EXTRAS_REQUIRE = {
    'reco': ['python-dateutil', 'simplejson'],
    'tests': [
        'pytest',
        'pytz',
    ],
    'lint': [
        'flake8==3.6.0',
        'pre-commit==1.12.0',
    ],
}

EXTRAS_REQUIRE['dev'] = (
    EXTRAS_REQUIRE['reco'] +
    EXTRAS_REQUIRE['tests'] +
    EXTRAS_REQUIRE['lint'] +
    ['tox']
)


def read(fname):
    with open(fname) as fp:
        content = fp.read()
    return content


setup(
    name='cmarshmallow',
    version='0.0.1',
    description=(
        'A fork of the marshmallow schema library focused on performance',
    ),
    long_description=read('README.rst'),
    author='Michael Bryant',
    author_email='mbryantj@gmail.com',
    url='https://github.com/mjbryant/cmarshmallow',
    packages=find_packages(exclude=('test*', 'examples')),
    package_dir={'cmarshmallow': 'cmarshmallow'},
    include_package_data=True,
    extras_require=EXTRAS_REQUIRE,
    license='MIT',
    zip_safe=False,
    keywords=(
        'serialization', 'rest', 'json', 'api', 'marshal',
        'marshalling', 'deserialization', 'validation', 'schema',
    ),
    classifiers=[
        'Development Status :: 5 - Production/Stable',
        'Intended Audience :: Developers',
        'License :: OSI Approved :: MIT License',
        'Programming Language :: Python :: 2',
        'Programming Language :: Python :: 2.7',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.5',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
    ],
    test_suite='tests',
    ext_modules=[Extension('cmarshmallow.marshaller', ['ext/marshaller.c'])]
)
