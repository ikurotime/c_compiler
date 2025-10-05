# Husk Language Specification

## Overview

Husk is a memory-safe, statically-typed programming language that combines the simplicity of Go with the safety of Rust, but with cleaner, more approachable syntax. It compiles to LLVM IR for high performance.

## Design Philosophy

1. **Memory Safe by Default** - No null pointers, buffer overflows, or data races
2. **Simple & Readable** - Inspired by Go's clarity, avoiding Rust's complexity
3. **Fast** - Compiles to optimized native code via LLVM
4. **Modern** - First-class functions, pattern matching, and type inference
5. **Practical** - Easy to write, easy to read, easy to maintain

## File Extension

`.hsk`

## Core Principles

### Memory Safety (without the ugly)
- No manual memory management
- No null pointers (use `Option<T>` instead)
- Ownership system (simpler than Rust's)
- Automatic cleanup (RAII-style)

### Go-like Simplicity
- Clean, minimal syntax
- Fast compilation
- Simple concurrency (goroutine-style)
- No complex generic syntax

---

## Type System

### Primitive Types
```husk
int       // 64-bit signed integer (default)
i32       // 32-bit signed integer
i64       // 64-bit signed integer
float     // 64-bit floating point
bool      // true or false
string    // UTF-8 string
```

### Type Inference (Go-style)
```husk
let x = 42;           // inferred as int
let name = "Alice";   // inferred as string
let pi = 3.14;        // inferred as float
```

### Explicit Types (when needed)
```husk
let x: i32 = 42;
let ratio: float = 0.5;
```

---

## Variables & Constants

### Immutable by Default (Rust-inspired)
```husk
let x = 5;       // immutable
x = 10;          // ERROR: cannot reassign
```

### Mutable Variables
```husk
let mut x = 5;   // mutable
x = 10;          // OK
```

### Constants
```husk
const MAX_SIZE = 1000;
```

---

## Functions

### Simple Function Syntax (Go-style)
```husk
fn add(a: int, b: int) -> int {
    return a + b;
}

// Type inference on return
fn add(a: int, b: int) {
    a + b  // implicit return like Rust
}

// Shorter syntax
fn greet(name: string) {
    print("Hello, " + name);
}
```

### Multiple Return Values (Go-style)
```husk
fn divide(a: int, b: int) -> (int, bool) {
    if b == 0 {
        return (0, false);
    }
    return (a / b, true);
}

let (result, ok) = divide(10, 2);
```

---

## Memory Safety Features

### No Null, Use Option (Rust-style but cleaner)
```husk
fn find(arr: []int, target: int) -> Option<int> {
    for (i, val) in arr {
        if val == target {
            return Some(i);
        }
    }
    return None;
}

// Pattern matching for safety
match find(numbers, 42) {
    Some(idx) => print("Found at: " + idx),
    None => print("Not found")
}
```

### Result Type for Errors (Rust-inspired)
```husk
fn read_file(path: string) -> Result<string, Error> {
    // ... implementation
}

match read_file("data.txt") {
    Ok(content) => print(content),
    Err(e) => print("Error: " + e)
}
```

### Ownership (Simplified from Rust)
```husk
fn take_ownership(s: string) {
    print(s);
}  // s is dropped here

let text = "hello";
take_ownership(text);
// text is moved, can't use it anymore

// To keep using it, pass a reference
fn borrow(s: &string) {
    print(s);
}

let text = "hello";
borrow(&text);
print(text);  // OK, text is still valid
```

---

## Control Flow

### If Expressions (Rust-style)
```husk
let x = if condition { 10 } else { 20 };

// Classic style
if x > 10 {
    print("big");
} else if x > 5 {
    print("medium");
} else {
    print("small");
}
```

### Pattern Matching (Rust-style but cleaner)
```husk
match value {
    0 => print("zero"),
    1 | 2 => print("one or two"),
    3..10 => print("three to nine"),
    _ => print("something else")
}
```

### Loops

#### For Loop (Go-style)
```husk
// Range-based
for i in 0..10 {
    print(i);
}

// With array
for (i, val) in arr {
    print(i, val);
}

// Infinite loop
for {
    if condition { break; }
}
```

#### While Loop
```husk
while x < 10 {
    x += 1;
}
```

---

## Data Structures

### Arrays (Fixed Size)
```husk
let arr: [5]int = [1, 2, 3, 4, 5];
let first = arr[0];
```

### Slices (Dynamic, Go-style)
```husk
let nums = [1, 2, 3, 4, 5];
let slice = nums[1..3];  // [2, 3]
```

### Structs (Simple, Go-inspired)
```husk
struct Person {
    name: string,
    age: int,
}

let alice = Person {
    name: "Alice",
    age: 30
};

print(alice.name);
```

### Methods (Clean syntax)
```husk
struct Counter {
    count: int
}

impl Counter {
    fn new() -> Counter {
        Counter { count: 0 }
    }
    
    fn increment(&mut self) {
        self.count += 1;
    }
}

let mut c = Counter::new();
c.increment();
```

---

## Concurrency (Go-style simplicity)

### Goroutine-style Async
```husk
fn process(id: int) {
    print("Processing " + id);
}

// Spawn async task
spawn process(1);
spawn process(2);

// With channels (Go-style)
let ch = channel<int>();

spawn {
    ch.send(42);
};

let value = ch.receive();
print(value);
```

---

## Error Handling

### Result Type (Better than try/catch)
```husk
fn divide(a: int, b: int) -> Result<int, string> {
    if b == 0 {
        return Err("division by zero");
    }
    return Ok(a / b);
}

// ? operator for propagation (Rust-style)
fn calculate() -> Result<int, string> {
    let x = divide(10, 2)?;
    let y = divide(20, x)?;
    return Ok(y);
}
```

---

## Standard Library

### Built-in Functions
```husk
print(value)          // Print to stdout
println(value)        // Print with newline
assert(condition)     // Runtime assertion
panic(message)        // Abort with error
len(collection)       // Length of array/slice/string
```

### String Operations
```husk
let s = "hello";
let upper = s.to_upper();
let chars = s.chars();
let joined = ["a", "b", "c"].join(",");
```

### Collections
```husk
let v = vec![1, 2, 3];
v.push(4);
v.pop();
let first = v.first();
```

---

## Example Programs

### Hello World
```husk
fn main() {
    print("Hello, World!");
}
```

### Fibonacci (Safe & Clean)
```husk
fn fib(n: int) -> int {
    if n <= 1 { return n; }
    return fib(n - 1) + fib(n - 2);
}

fn main() {
    print(fib(10));
}
```

### Safe File Reading
```husk
fn main() {
    match read_file("data.txt") {
        Ok(content) => {
            let lines = content.split("\n");
            for line in lines {
                print(line);
            }
        },
        Err(e) => print("Error: " + e)
    }
}
```

### Concurrent Server (Go-style)
```husk
fn handle_request(req: Request) {
    print("Handling: " + req.path);
}

fn main() {
    let server = Server::new("127.0.0.1:8080");
    
    for req in server.listen() {
        spawn handle_request(req);
    }
}
```

---

## Current Implementation Status (v0.1)

### Implemented ✅
- [x] Lexer/Tokenizer
- [x] Basic Parser
- [x] LLVM Code Generation
- [x] Integer literals
- [x] Addition operator (+)
- [x] Print function

### Next Up 🚧
- [ ] More operators (-, *, /, %)
- [ ] Operator precedence
- [ ] Parentheses
- [ ] Variables (let)
- [ ] Mutable variables (let mut)
- [ ] Functions (fn)
- [ ] If expressions
- [ ] Match expressions
- [ ] Structs
- [ ] Option<T> and Result<T, E>
- [ ] Ownership & borrowing
- [ ] Async/spawn

---

## Grammar (Current)

```ebnf
Program     ::= Statement*
Statement   ::= PrintStmt | ExprStmt
PrintStmt   ::= "print" "(" Expression ")" ";"
ExprStmt    ::= Expression ";"
Expression  ::= Term ( "+" Term )*
Term        ::= IntLiteral
IntLiteral  ::= [0-9]+
```

## Grammar (Target Full Version)

```ebnf
Program     ::= Item*
Item        ::= Function | Struct | Impl

Function    ::= "fn" Identifier "(" Params? ")" ("->" Type)? Block

Struct      ::= "struct" Identifier "{" Fields "}"
Impl        ::= "impl" Identifier "{" Function* "}"

Statement   ::= LetStmt | IfExpr | Match | For | While 
             |  Return | Print | ExprStmt

LetStmt     ::= "let" "mut"? Identifier (":" Type)? "=" Expression ";"
IfExpr      ::= "if" Expression Block ("else" Block)?
Match       ::= "match" Expression "{" MatchArm* "}"
For         ::= "for" Pattern "in" Expression Block
While       ::= "while" Expression Block
Return      ::= "return" Expression ";"
Print       ::= "print" "(" Expression ")" ";"

Expression  ::= Assignment | LogicalOr
...
```

---

## Comparison with Go and Rust

| Feature | Husk | Go | Rust |
|---------|------|----|----- |
| Memory Safety | ✅ Ownership | ❌ GC | ✅ Ownership |
| Syntax | Clean | Clean | Complex |
| Null Safety | ✅ Option | ❌ nil | ✅ Option |
| Error Handling | Result | Multiple return | Result |
| Generics | Simple | Simple | Complex |
| Concurrency | Go-style | Goroutines | Async/Await |
| Learning Curve | Easy | Easy | Hard |
| Performance | Native (LLVM) | GC Overhead | Native |

**Husk = Go's simplicity + Rust's safety - Rust's complexity**

---

## Version History

### v0.1.0 (Current - January 2025)
- Basic arithmetic with + operator
- print() function  
- Integer literals
- LLVM backend
- Project foundation laid

### Roadmap
- **v0.2**: Full arithmetic, variables, functions
- **v0.3**: Structs, methods, ownership
- **v0.4**: Pattern matching, Result/Option
- **v0.5**: Concurrency (spawn, channels)
- **v1.0**: Complete language with standard library
