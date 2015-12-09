all: docs

# generate documentation
docs:
	doxygen
	@cat doxywarn.log

# clean all generated files
clean:
	rm -rf ./html/search
	rm -rf ./html/*.html
	rm -rf ./html/*.png
	rm -rf ./html/*.css
	rm -rf ./html/*.js


repo="https://github.com/devicehive/devicehive-cpp.git"

# initialize gh-pages repo
repo-init:
	rm -rf ./html
	git clone -b gh-pages --single-branch ${repo} ./html

# push updated gh-pages
repo-push: clean docs
	pushd ./html
	git add .
	git commit -m "documentation updated"
	git push origin gh-pages
	popd

.PHONY: clean docs
