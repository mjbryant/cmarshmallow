.PHONY: build

build-ext:
	python setup.py build
	cp build/lib.macosx-10.13-x86_64-3.5/cmarshmallow/marshaller.cpython-35m-darwin.so \
		marshaller.cpython-35m-darwin.so

build:
	CFLAGS="-O0 -g" python3 setup.py build
	cp build/lib.linux-x86_64-3.6/cmarshmallow/marshaller.cpython-36m-x86_64-linux-gnu.so \
		marshaller.so

test: build-ext
	python test_c.py
