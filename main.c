#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h> //zalecenie dodania przy kompilacji
#include <string.h> //zalecenie dodania przy kompilacji
#define MAX_BUFFER 1024
#define FILE_NAME_A "/proc/sykom/rejdoladaA" // plik do zapisu A
#define FILE_NAME_S "/proc/sykom/rejdoladaS" // plik do odczytu stanu S
#define FILE_NAME_W "/proc/sykom/rejdoladaW" // plik do odczytu wyniku W

// funkcja zapisu do A
void write_to_A(char *buffer, int fd_A, int fd_W, int fd_S, int i)
{
    snprintf(buffer, MAX_BUFFER, "%x", i);       // treść do przekazania sterownikowi
    lseek(fd_A, 0L, SEEK_SET);                   // przesunięcie się na początek pliku
    int n = write(fd_A, buffer, strlen(buffer)); // właściwe przekazanie danych
    if (n != strlen(buffer))
    {
        printf("Błąd zapisu do pliku %s - error: %d\n", FILE_NAME_A, errno);
        close(fd_A);
        close(fd_W);
        close(fd_S);
        exit(4);
    }
    else
    {
        printf("Poprawnie zapisano wartość 0x%x do rejestru A.\n", i);
    }
}

// funkcja odczytu S (bez pętli)
void read_from_S(char *buffer, int fd_A, int fd_W, int fd_S)
{
    lseek(fd_S, 0L, SEEK_SET);              // przesunięcie się na początek pliku
    int n = read(fd_S, buffer, MAX_BUFFER); // odczyt danych ze sterownika
    if (n > 0)
    {
        buffer[n] = '\0';
        printf("Stan automatu wynosi 0x%s\n", buffer);
    }
    else
    {
        printf("Błąd odczytu z pliku %s - error: %d\n", FILE_NAME_S, errno);
        close(fd_A);
        close(fd_W);
        close(fd_S);
        exit(5);
    }
}

// funkcja odczytu S w pętli
void read_from_S_loop(char *buffer, int fd_A, int fd_W, int fd_S)
{
    // Pętla czekająca dopóki automat nie osiągnie stanu 0xdd (DONE)
    for (;;)
    {
        lseek(fd_S, 0L, SEEK_SET);              // przesunięcie się na początek pliku
        int n = read(fd_S, buffer, MAX_BUFFER); // odczyt danych ze sterownika
        if (n > 0)
        {
            buffer[n] = '\0';
            if (buffer[0] == 'd' && buffer[1] == 'd')
            {
                printf("Stan automatu wynosi 0x%s", buffer);
                break;
            }
        }
        else
        {
            printf("Błąd odczytu z pliku %s - error: %d\n", FILE_NAME_S, errno);
            close(fd_A);
            close(fd_W);
            close(fd_S);
            exit(5);
        }
    }
    /*Pętla czekająca aż po obliczeniu wyniku automat wróci do stanu IDLE (aa) -
    zabezpieczenie przed desynchronizacją w czasie testów (wartość dd wypisze się
    kiklka razy więc po zatrzymaniu pętli po wykryciu pierwszego wystąpienie może
    nastąpić desynchronizacja*/
    for (;;)
    {
        lseek(fd_S, 0L, SEEK_SET);              // przesunięcie się na początek pliku
        int n = read(fd_S, buffer, MAX_BUFFER); // odczyt danych ze sterownika
        if (n > 0)
        {
            buffer[n] = '\0';
            if (buffer[0] == 'a' && buffer[1] == 'a')
            {
                printf("Stan automatu wynosi 0x%s\n", buffer);
                break;
            }
        }
        else
        {
            printf("Błąd odczytu z pliku %s - error: %d\n", FILE_NAME_S, errno);
            close(fd_A);
            close(fd_W);
            close(fd_S);
            exit(5);
        }
    }
}

// funkcja odczytu W
void read_from_W(char *buffer, int fd_A, int fd_W, int fd_S)
{
    lseek(fd_W, 0L, SEEK_SET);              // przesunięcie się na początek pliku
    int n = read(fd_W, buffer, MAX_BUFFER); // odczyt danych ze sterownika
    if (n > 0)
    {
        buffer[n] = '\0';
        printf("Liczona liczba pierwsza wynosi: 0x%s\n", buffer);
    }
    else
    {
        printf("Błąd odczytu z pliku %s - error: %d\n", FILE_NAME_W, errno);
        close(fd_A);
        close(fd_W);
        close(fd_S);
        exit(6);
    }
}

/*Funkcja oczekiwania na zainicjowanie (do testów wpisania 2 liczb po kolei
od razu bez doliczenia pierwszej do końca */
void wait_for_S_INIT(char *buffer, int fd_A, int fd_W, int fd_S)
{
    /*Pętla oczekująca na stan 0xbb (INIT), który oznacza rozpoczęcia liczenia dla liczby z wewnętrznego
    rejestru system_A - można wtedy podać nową liczbę do kolejnego wyliczenia*/
    for (;;)
    {
        lseek(fd_S, 0L, SEEK_SET);              // przesunięcie się na początek pliku
        int n = read(fd_S, buffer, MAX_BUFFER); // odczyt danych ze sterownika
        if (n > 0)
        {
            buffer[n] = '\0';
            if (buffer[0] == 'b' && buffer[1] == 'b')
            {
                printf("Stan automatu wynosi 0x%s", buffer);
                break;
            }
        }
        else
        {
            printf("Błąd odczytu z pliku %s - error: %d\n", FILE_NAME_S, errno);
            close(fd_A);
            close(fd_W);
            close(fd_S);
            exit(5);
        }
    }
}

// Główna funkcja programu
int main(void)
{
    // Otwieranie plików
    char buffer[MAX_BUFFER];
    int i;                                // wartość do wyliczenia
    int k;                                // do pętli w końcowej fazie testów
    int fd_A = open(FILE_NAME_A, O_RDWR); // otwarcie pliku "A" sterownika

    if (fd_A < 0)
    {
        printf("Nie udało się otworzyć pliku %s - error: %d\n", FILE_NAME_A, errno);
        exit(1);
    }

    int fd_W = open(FILE_NAME_W, O_RDWR); // otwarcie pliku "W" sterownika
    if (fd_W < 0)
    {
        printf("Nie udało się otworzyć pliku %s - error: %d\n", FILE_NAME_W, errno);
        close(fd_A); // zamykanie poprzednio otwartego pliku A w razie błędu
        exit(2);
    }

    int fd_S = open(FILE_NAME_S, O_RDWR); // otwarcie pliku "S" sterownika
    if (fd_S < 0)
    {
        printf("Nie udało się otworzyć pliku %s - error: %d\n", FILE_NAME_S, errno);
        close(fd_A); // zamykanie poprzednio otwartego pliku A w razie błędu
        close(fd_W); // zamykanie poprzednio otwartego pliku W w razie błędu
        exit(3);
    }

    printf("\n\n---------------- POCZĄTEK TESTÓW ----------------\n\n");

    // ----- TEST 1: liczba 0x18 (24) ----

    printf("\n----------- TEST 1: liczba 0x18 (24 dziesiętnie) -----------\n\n");

    // Zapis liczby 0x18 (24 dziesiętnie)
    i = 24;
    write_to_A(buffer, fd_A, fd_W, fd_S, i);

    /*Pętla czekająca, aż moduł skończy liczyć - odczyt stanu S aż do uzyskania "0xdd" (stan DONE)
    oznaczający koniec liczenia*/
    printf("Rozpoczynam liczenie 0x%x (%d) liczby pierwszej...\n", i, i);
    read_from_S_loop(buffer, fd_A, fd_W, fd_S);

    /*Odczyt wartości liczby pierwszej z rejestru W*/
    read_from_W(buffer, fd_A, fd_W, fd_S);

    // ----- TEST 2: liczba 0x3E8 (1000) ----

    printf("\n----------- TEST 2: liczba 0x3E8 (1000 dziesiętnie) -----------\n\n");

    // Zapis liczby 0x3E8  (1000 dziesiętnie)
    i = 1000;
    write_to_A(buffer, fd_A, fd_W, fd_S, i);

    printf("Rozpoczynam liczenie 0x%x (%d) liczby pierwszej...\n", i, i);
    read_from_S_loop(buffer, fd_A, fd_W, fd_S);

    read_from_W(buffer, fd_A, fd_W, fd_S);

    // ----- TEST 3: liczba 0x1 (1) ----

    printf("\n----------- TEST 3: liczba 0x1 (1 dziesiętnie) -----------\n\n");

    // Zapis liczby 0x1  (1 dziesiętnie)
    i = 1;
    write_to_A(buffer, fd_A, fd_W, fd_S, i);

    printf("Rozpoczynam liczenie 0x%x (%d) liczby pierwszej...\n", i, i);
    read_from_S_loop(buffer, fd_A, fd_W, fd_S);

    read_from_W(buffer, fd_A, fd_W, fd_S);

    printf("\n----------- TEST 4: wpisanie takich samych wartości po kolei -----------\n\n");

    // Zapis liczby 0x80  (128 dziesiętnie)
    i = 128;
    write_to_A(buffer, fd_A, fd_W, fd_S, i);

    printf("Rozpoczynam liczenie 0x%x (%d) liczby pierwszej...\n", i, i);
    read_from_S_loop(buffer, fd_A, fd_W, fd_S);

    read_from_W(buffer, fd_A, fd_W, fd_S);

    printf("\n---- Ponowny zapis tej samej wartości ----\n\n");

    // Ponowny zapis liczby 0x80
    write_to_A(buffer, fd_A, fd_W, fd_S, i);

    // Odczyt stanu (tym razem bez pętli - tutaj nigdy nie osiągniemy stanu DONE, więc działałaby w nieskończoność)
    read_from_S(buffer, fd_A, fd_W, fd_S);

    // Odczyt wyniku - powinien być niezmieniony
    read_from_W(buffer, fd_A, fd_W, fd_S);

    printf("\n----------- TEST 5: wpisanie dwóch różnych wartości od razu po sobie -----------\n\n");

    // Zapis liczby 0xFA (250 dziesiętnie)
    i = 250;
    write_to_A(buffer, fd_A, fd_W, fd_S, i);

    printf("Oczekujemy na stan 0xbb - INIT...\n");

    wait_for_S_INIT(buffer, fd_A, fd_W, fd_S);

    printf("\nRozpoczynam liczenie 0x%x (%d) liczby pierwszej...\n\n", 250, 250);
	// Zapis liczby 0x170  (368 dziesiętnie)
    i = 368;
    write_to_A(buffer, fd_A, fd_W, fd_S, i);

    read_from_S_loop(buffer, fd_A, fd_W, fd_S);

    read_from_W(buffer, fd_A, fd_W, fd_S);

    printf("\nRozpoczynam liczenie 0x%x (%d) liczby pierwszej...\n", 368, 368);
    read_from_S_loop(buffer, fd_A, fd_W, fd_S);

    read_from_W(buffer, fd_A, fd_W, fd_S);

    printf("\n----------- TEST 6: liczba 0x0 (0 dziesiętnie) -----------\n\n");

    // Zapis liczby 0x0  (0 dziesiętnie)
    i = 0;
    write_to_A(buffer, fd_A, fd_W, fd_S, i);

    printf("Rozpoczynam liczenie 0x%x (%d) liczby pierwszej...\n", i, i);

    printf("Dla liczby %x stan pozostaje w stanie 0xaa (IDLE)\n", i);

    for (k = 0; k < 10; k++)
    {
        read_from_S(buffer, fd_A, fd_W, fd_S);
    }
    printf("Wynik pozostaje niezmieniony (dla poprzedniej liczonej liczby pierwszej)\n");
    read_from_W(buffer, fd_A, fd_W, fd_S);

    printf("\n----------- TEST 7: liczba 0x3E9 (1001 dziesiętnie) -----------\n\n");

    // Zapis liczby 0x3E9  (1001 dziesiętnie)
    i = 1001;
    write_to_A(buffer, fd_A, fd_W, fd_S, i);

    printf("Rozpoczynam liczenie 0x%x (%d) liczby pierwszej...\n", i, i);

    printf("Dla liczby %x stan pozostaje w stanie 0xaa (IDLE)\n", i);
    for (k = 0; k < 10; k++)
    {
        read_from_S(buffer, fd_A, fd_W, fd_S);
    }

    printf("Wynik pozostaje niezmieniony (dla poprzedniej liczonej liczby pierwszej)\n");
    read_from_W(buffer, fd_A, fd_W, fd_S);

    /*Dla testów 8-10 w osbłudze błędó nie zamykam wszystkich plików i nie wychodzę z aplikacji, aby sprawdzić
    zapis/odczyt dla wybranych plików. W razie ich zamknięcia otrzymałym inny kod błędu (9) niezwiązany
    błędem I/O - kod błędu 5*/

    printf("\n----------- TEST 8: próba odczytu z rejestru A -----------\n\n");

    lseek(fd_A, 0L, SEEK_SET);               // przesunięcie się na początek pliku
    int nA = read(fd_A, buffer, MAX_BUFFER); // odczyt danych ze sterownika
    if (nA > 0)
    {
        buffer[nA] = '\0';
        printf("Stan automatu wynosi %s\n", buffer);
    }
    else
    {
        printf("Błąd odczytu z pliku %s - error: %d\n", FILE_NAME_A, errno);
    }

    printf("\n----------- TEST 9: próba zapisu do rejestru W -----------\n\n");

    snprintf(buffer, MAX_BUFFER, "%x", i);        // treść do przekazania sterownikowi
    lseek(fd_W, 0L, SEEK_SET);                    // przesunięcie się na początek pliku
    int nW = write(fd_W, buffer, strlen(buffer)); // właściwe przekazanie danych
    if (nW != strlen(buffer))
    {
        printf("Błąd zapisu do pliku %s - error: %d\n", FILE_NAME_W, errno);
    }
    else
    {
        printf("Poprawnie zapisano wartość %x do rejestru W.\n", i);
    }

    printf("\n----------- TEST 10: próba zapisu do rejestru S -----------\n\n");

    snprintf(buffer, MAX_BUFFER, "%x", i);        // treść do przekazania sterownikowi
    lseek(fd_S, 0L, SEEK_SET);                    // przesunięcie się na początek pliku
    int nS = write(fd_S, buffer, strlen(buffer)); // właściwe przekazanie danych
    if (nS != strlen(buffer))
    {
        printf("Błąd zapisu do pliku %s - error: %d\n", FILE_NAME_S, errno);
    }
    else
    {
        printf("Poprawnie zapisano wartość %x do rejestru W.\n", i);
    }

    printf("\n\n---------------- KONIEC TESTÓW ----------------\n\n");

    // Zamknięcie wszystkich używanych plików
    close(fd_A);
    close(fd_S);
    close(fd_W);
    return 0;
}