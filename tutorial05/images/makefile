%.png: %.dot
	dot $< -Tpng -o $@

DOTFILES = $(basename $(wildcard *.dot))
all: $(addsuffix .png, $(DOTFILES))
