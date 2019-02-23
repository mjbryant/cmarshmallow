clean:
	rm -rf build

build: clean
	python c_setup.py build
	cp build/lib.macosx-10.13-x86_64-3.5/marshaller.cpython-35m-darwin.so .
