# Ép latexmk (và VimTeX) dùng XeLaTeX vì tài liệu dùng fontspec + tiếng Việt
$pdf_mode = 5;          # 5 = xelatex
$xelatex = 'xelatex -interaction=nonstopmode -synctex=1 %O %S';
$bibtex_use = 2;        # tự chạy bibtex cho tài liệu tham khảo

# Chỉ biên dịch main.tex khi chạy `latexmk` không tham số
# (tránh việc latexmk cố dịch các file con như titlepage.tex / chapters/*)
@default_files = ('main.tex');
