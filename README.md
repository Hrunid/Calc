# Parser i kalkulator wyrażeń arytmetycznych

---

## 1. Opis projektu

Projekt przedstawia model parsera i kalkulatora wyrażeń arytmetycznych, który oprócz podstawowych działań, takich jak dodawanie, odejmowanie, mnożenie i dzielenie, obsługuje również bardziej zaawansowane funkcjonalności, takie jak definiowanie zmiennych, potęgowanie oraz podstawowe funkcje matematyczne: `sin`, `cos`, `tan`, `ln`, `abs`, `sqrt`.

Kalkulator można opisać jako funkcję:

```text
Calc: string -> number
```

lub:

```text
Calc: string -> error
```

w zależności od tego, czy ciąg wejściowy był poprawny składniowo i możliwy do obliczenia.

Program działa w trybie interaktywnym. Użytkownik podaje kolejne wyrażenia lub instrukcje, a aplikacja zwraca wynik albo listę wykrytych błędów. Zdefiniowane zmienne są przechowywane pomiędzy kolejnymi wejściami.

---

## 2. Model języka i gramatyki

### 2.1. Formalna definicja gramatyki bezkontekstowej

Gramatyką jest zbiór:

```text
G = {V, T, P, S}
```

Dla tego języka zbiór terminali to:

```text
T = { '+', '-', '*', '/', '^', '=', ':=', ';', '.', '(', ')', Digits, Letters, Functions }
```

gdzie:

```text
Digits = {0,1,2,3,4,5,6,7,8,9}
```
to zbiór cyfr systemu dziesiętnego.

```text
Letters = {a,b,c,…,x,y,z,A,B,C,…,X,Y,Z}
```

to małe i wielkie litery alfabetu łacińskiego.

```text
Functions = {sin, cos, tan, ln, sqrt, abs}
```

to nazwy dostępnych funkcji matematycznych.

Zbiór nieterminali:

```text
V = {
    S,
    Program,
    InstructionList,
    Instruction,
    VariableDef,
    Expression,
    Add,
    Mult,
    Unary,
    Power,
    Primary,
    VariableRef,
    Number,
    Float,
    Integer,
    FunctionCall,
    D,
    L,
    F
}
```

Symbolem startowym jest:

```text
S
```

Zasady produkcji `P`:

```text
S -> Program

Program -> InstructionList

InstructionList -> Instruction ; InstructionList | Instruction ; | Instruction

Instruction -> VariableDef | Expression

VariableDef -> VariableRef := Expression | VariableRef = Expression

Expression -> Add

Add -> Add + Mult | Add - Mult | Mult

Mult -> Mult * Unary | Mult / Unary | Unary

Unary -> Power | - Unary

Power -> Primary ^ Unary | Primary

Primary -> Number | VariableRef | FunctionCall | ( Expression )

VariableRef -> L | LInteger

Number -> Integer | Float

Float -> Integer.Integer

Integer -> D | IntegerD

FunctionCall -> F ( Expression )

D -> 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

L -> a | b | c | ... | z | A | B | C | ... | Z

F -> sin | cos | tan | ln | sqrt | abs
```

Gramatyka jest gramatyką bezkontekstową, czyli należy do typu 2 w hierarchii Chomsky’ego. Każda produkcja ma po lewej stronie pojedynczy symbol nieterminalny.

Gramatyka uwzględnia klasyczną kolejność działań. Najniższy priorytet mają dodawanie i odejmowanie, następnie mnożenie i dzielenie, następnie operator jednoargumentowy minus, a najwyższy priorytet ma potęgowanie oraz nawiasowanie. Dzięki temu wyrażenie:

```text
-2^2
```

jest interpretowane jako:

```text
-(2^2)
```

natomiast:

```text
(-2)^2
```

oznacza potęgowanie liczby `-2`.

### 2.2. Nieformalna implementacja języka

Implementacja bazuje na praktycznym modelu wykorzystującym tokeny i drzewo AST.

Tokenem jest każde niezależne słowo. Niezależnymi słowami są spójne ciągi znaków rozdzielone znakami białymi lub jednym z separatorów:

```text
{ +, -, *, /, ^, =, :=, ;, ., (, ) }
```

przy czym każdy z tych separatorów również jest niezależnym tokenem.

Każdy token jest jednym z następujących typów:

- `Operator` – wszystkie operatory arytmetyczne: `+`, `-`, `*`, `/`, `^`,
- `Integer` – ciąg cyfr dziesiętnych,
- `Function` – nazwa dozwolonej funkcji,
- `Variable` – nazwa zmiennej zgodna z gramatyką,
- `Parenthesis` – nawias okrągły `(` lub `)`,
- `Separator` – znak `;` lub `.`,
- `Assign` – operator przypisania `=` lub `:=`,
- `Unknown` – nierozpoznany wzorzec; w poprawnym gramatycznie wyrażeniu nie powinien wystąpić.

Oprócz tokenów istotną strukturą są węzły AST, które mogą być jednego z typów:

- `Program` – korzeń drzewa reprezentujący cały program,
- `InstructionList` – lista instrukcji oddzielonych średnikiem,
- `VariableDefinition` – instrukcja przypisania wartości do zmiennej,
- `Expression` – wyrażenie arytmetyczne,
- `Add` – poziom dodawania i odejmowania,
- `Mult` – poziom mnożenia i dzielenia,
- `Unary` – poziom operatorów jednoargumentowych, np. `-2`,
- `Power` – poziom potęgowania,
- `Number` – liczba złożona z tokenu `Integer` albo sekwencji `Integer`, `Separator(".")`, `Integer`,
- `VariableReference` – odwołanie do zmiennej,
- `FunctionCall` – wyrażenie postaci `Function(Expression)`,
- `Error` – węzeł oznaczający fragment niepoprawny składniowo.

---

## 3. Struktura implementacji

Projekt został podzielony na kilka klas i struktur, z których każda odpowiada za osobny etap przetwarzania wejścia.

### 3.1. Token

Struktura `Token` reprezentuje pojedynczy element wejścia rozpoznany przez lekser. Token przechowuje swój typ, wartość tekstową oraz pozycję w oryginalnym ciągu wejściowym. Pozycja jest istotna przy zgłaszaniu błędów, ponieważ pozwala wskazać dokładne miejsce wystąpienia problemu.

Tokeny nie zawierają informacji o znaczeniu całego wyrażenia. Są jedynie wynikiem analizy leksykalnej.

### 3.2. Lexer

Klasa `Lexer` odpowiada za analizę leksykalną. Jej zadaniem jest zamiana ciągu znaków podanego przez użytkownika na uporządkowany wektor tokenów.

Lekser rozpoznaje operatory, liczby całkowite, nazwy funkcji, zmienne, nawiasy, separatory oraz operatory przypisania. Jeżeli fragment wejścia nie pasuje do żadnego znanego wzorca, zostaje oznaczony jako token typu `Unknown`.

Lekser nie sprawdza poprawności gramatycznej całego wyrażenia. Przykładowo może poprawnie rozpoznać tokeny w napisie:

```text
x = 1 + ;
```

ale nie decyduje, czy takie wyrażenie jest poprawne składniowo. Tym zajmuje się parser.

### 3.3. AST

Klasa `AST` przechowuje abstrakcyjne drzewo składniowe programu. Drzewo składa się z węzłów typu `Node`.

Każdy węzeł zawiera:

- typ węzła,
- listę tokenów związanych z tym węzłem,
- listę dzieci, czyli węzłów podrzędnych.

W implementacji zastosowano jeden uniwersalny typ węzła zamiast hierarchii klas dziedziczących po sobie. O znaczeniu węzła decyduje wartość pola `NodeType`.

AST upraszcza strukturę programu względem pełnego drzewa parsowania. Nawiasy, średniki i niektóre techniczne elementy gramatyki nie muszą być osobnymi węzłami, jeżeli ich rola została już uwzględniona w strukturze drzewa.

### 3.4. Parser

Klasa `Parser` odpowiada za analizę składniową. Otrzymuje wektor tokenów i próbuje zbudować z niego poprawne drzewo AST.

Parser rozpoznaje strukturę programu zgodnie z gramatyką:

```text
Program -> InstructionList
Instruction -> VariableDef | Expression
Expression -> Add
Add -> Mult
Mult -> Unary
Unary -> Power
Power -> Primary
```

Parser odpowiada również za wykrywanie błędów leksykalnych i składniowych. Zgłasza między innymi:

- użycie tokenu `Unknown`,
- brak wyrażenia po operatorze,
- brak prawej strony przypisania,
- niepoprawne nawiasowanie,
- niepoprawną postać wywołania funkcji,
- niepoprawną postać liczby,
- występowanie dwóch wyrażeń bez operatora między nimi.

W przypadku błędu parser nie powinien kończyć programu awaryjnie. Zamiast tego zapisuje informacje o błędzie w wektorze `Error` i, jeżeli to możliwe, buduje częściowe AST z węzłami typu `Error`.

Parser nie oblicza wartości wyrażeń i nie sprawdza, czy zmienna została wcześniej zainicjowana. Takie błędy są obsługiwane na etapie ewaluacji.

### 3.5. Error

Struktura `Error` reprezentuje informację o błędzie wykrytym podczas działania programu. Każdy błąd zawiera:

- typ błędu,
- pozycję w tekście wejściowym,
- długość problematycznego fragmentu,
- komunikat opisujący problem.

W projekcie wyróżniono następujące typy błędów:

- `UnknownToken` – nierozpoznany token,
- `SyntaxError` – błąd składniowy,
- `UninitializedVariable` – użycie zmiennej bez wcześniejszego przypisania wartości,
- `ValueError` – błąd wartości, np. `sqrt(-1)` albo `ln(0)`.

Parser zgłasza błędy typu `UnknownToken` i `SyntaxError`. Błędy `UninitializedVariable` oraz `ValueError` są zgłaszane przez ewaluator.

### 3.6. Evaluator

Klasa `Evaluator` odpowiada za obliczanie wartości drzewa AST. Otrzymuje poprawne składniowo AST oraz mapę zmiennych.

Evaluator obsługuje działania arytmetyczne, potęgowanie, operator jednoargumentowy minus, wywołania funkcji oraz przypisania do zmiennych. W przypadku instrukcji przypisania oblicza wartość prawej strony i zapisuje ją w mapie zmiennych.

Mapa zmiennych jest przechowywana poza ewaluatorem, dzięki czemu jej zawartość może być zachowana między kolejnymi wejściami użytkownika.

Evaluator odpowiada za błędy semantyczne i wartościowe, takie jak użycie niezainicjowanej zmiennej, dzielenie przez zero, pierwiastek z liczby ujemnej lub logarytm z liczby niedodatniej.

### 3.7. App

Klasa `App` zarządza całym przebiegiem programu. Jest warstwą łączącą lekser, parser i ewaluator.

Jej zadania to:

- pobieranie wejścia od użytkownika,
- uruchomienie leksera,
- przekazanie tokenów do parsera,
- wypisanie błędów składniowych, jeżeli wystąpiły,
- przekazanie AST do ewaluatora,
- wypisanie błędów ewaluacji, jeżeli wystąpiły,
- wypisanie wyniku,
- przechowywanie mapy zmiennych między kolejnymi wejściami.

Dzięki temu parser nie zajmuje się wejściem ani obliczaniem wartości, a ewaluator nie zajmuje się analizą składniową. Klasa `App` odpowiada za kontrolę przepływu programu.

---

## 4. Instrukcja uruchomienia

Projekt jest napisany w języku C++ i korzysta z systemu budowania CMake.

Wymagania:

- kompilator C++ obsługujący standard C++17,
- CMake w wersji co najmniej 3.16(opcjonalne)



Przykład kompilacji z wykorzystaniem g++. W wierszu poleceń, w katalogu z kodem źródłowym wykonać polecenie:

g++ -std=c++17 *.cpp -o program

gdzie argument po parametrze -o, w tym przypadku "./program" to miejsce outputu binarnego, skompilowanego kodu.
W przypadku brak g++ w systemie Linux można go zainstalować poleceniem apt:

apt install g++

Przykład kompilacji z CMake

```bash
cmake -S . -B build
cmake --build build
```

Po zbudowaniu program można uruchomić poleceniem:

```bash
./build/calc_app
```

W systemie Windows plik wykonywalny może znajdować się na przykład w katalogu:

```bash
.\\build\\Debug\\calc_app.exe
```

Alternatywnie w katalogu ./build znadziemy dwa gotowe skompilowane wersje odpowienio po system Windows10 oraz Linux. W tym przypadku wystarczy uruchomic, jak zwykły program.

---

## 5. Przykłady użycia

### 5.1. Poprawne wyrażenie arytmetyczne

Wejście:

```text
2 + 3 * 4
```

Wynik:

```text
14
```

Wyrażenie jest interpretowane zgodnie z kolejnością działań:

```text
2 + (3 * 4)
```

### 5.2. Zmienne i kolejne instrukcje

Wejście:

```text
x = 5; y = x^2; y + 1
```

Wynik:

```text
26
```

Program najpierw przypisuje wartość `5` do zmiennej `x`, następnie oblicza `x^2` i zapisuje wynik do `y`, a na końcu zwraca wartość wyrażenia `y + 1`.

### 5.3. Funkcje matematyczne

Wejście:

```text
sqrt(16) + abs(-3)
```

Wynik:

```text
7
```

### 5.4. Błąd składniowy

Wejście:

```text
x = 1 + ;
```

Przykładowy komunikat błędu:

```text
SyntaxError at position 8: Expected expression after operator '+'.
x = 1 + ;
        ^
```

Parser rozpoznaje, że po operatorze `+` powinno znajdować się kolejne wyrażenie.

### 5.5. Błąd użycia niezainicjowanej zmiennej

Wejście:

```text
x + 1
```

Jeżeli zmienna `x` nie została wcześniej zdefiniowana, evaluator zwróci błąd:

```text
UninitializedVariable: Variable 'x' was used before assignment.
```

### 5.6. Błąd wartości

Wejście:

```text
sqrt(-1)
```

Wynik:

```text
ValueError: Function 'sqrt' is not defined for negative values.
```
### Autor: Wiktor Wdowczyk