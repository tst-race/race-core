# -----------------------------------------------------------------------------
# Makefile for the Comms Python plugin/examplar
#
# Commands:
# help                        Print help documentation
# black                       Run Python's Black formatter on the code
# black_check                 Run Python's Black formatter on the code in check mode. Fail if black needs to be run. Good for CI.
# mypy                        Run Python's Mypy type checker on the code
# pylint                      Run Python's Pylint syntax checker on the code
# pylint_check                Run Python's Pylint syntax checker on the code and fail if score fails minimum threshold. Good for CI.
# todo                        Get project TODOs
# -----------------------------------------------------------------------------


###
# Help/Setup
###


# Make phony commands
.PHONY: help black black_check mypy pylint pylint_check todo

help:
	# Print help documentation
	@echo "This makefile holds the following targets"
	@echo "  help                        Print help documentation"
	@echo "  black                       Run Python's Black formatter on the code"
	@echo "  black_check                 Run Python's Black formatter on the code in check mode. Fail if black needs to be run. Good for CI."
	@echo "  mypy                        Run Python's Mypy type checker on the code"
	@echo "  pylint                      Run Python's Pylint syntax checker on the code"
	@echo "  pylint_check                Run Python's Pylint syntax checker on the code and fail if score fails minimum threshold. Good for CI."
	@echo "  pycodestyle                 Run Python's pep8 style guide checker on the code"
	@echo "  todo                        Get project TODOs"


###
# Development/Standards Tools
###


todo:
	# Get all package TODOs
	grep -rE "TODO" source | egrep .py | egrep -v .html | egrep -v .eggs | egrep -v .pyc

mypy:
	# Run Mypi on the package
	mypy --config-file=.mypy.ini

pycodestyle:
	# Run Pycodestyle on the package
	pycodestyle --config=.pycodestylerc source

pylint:
	# Run Pylint on the package
	pylint source

pylint_check:
	# Run Pylint on the package with minimum required score
	pylint --fail-under=7.5 source

black:
	# Run Black on the package
	python3 -m black --config=.blackrc.toml source

black_check:
	# Run Black on the package in check mode
	python3 -m black --config=.blackrc.toml --check source

style: black
