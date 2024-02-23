# HTTP-Client
am ales sa creez o conexiune noua cu serverul la fiecare comanda
pentru a evita timeout-ul.
Functia parse command verifica ca toate comenzile pe care le primeste clientul 
sa fie valide, iar in cazul comenzii add_book am creat si functia validate_book
ca sa verific datele cartii a.i sa nu poti da un string gol la nume si numarul
de pagini sa fie un numar intreg.
La comanda get_books am creat si functia get_books care afiseaza datele
returnate de server prin json am facut niste parsari aditionale pentru
a pastra formatul de la add_book.
Am ales biblioteca json recomandata in enunt pentru a face crearea json-ului 
de trimis la server.

DETALIU IMPORTANT
!!!!!!!!!!!!!!!!!
Pentru ca o comanda sa fie valida trebuie sa fie fix de forma command\n
nu command    \n sau comandxyz\n. Orice caracter in plus fata de comanda si newline
va face comanda sa fie valida. Comenzile de forma comanda spatiu spatiu spatiu va fi invalida
Doar cele de forma comanda newline vor fi valide deoarece sterg newline la parsare
!!!!!!!!!!!!!!!!!
e.g get_books\n e o comanda valida
    get_books    \n e o comanda invalida
    get_books\n\n e o comanda invalida
    get_booksasdads\n e o comanda invalida
