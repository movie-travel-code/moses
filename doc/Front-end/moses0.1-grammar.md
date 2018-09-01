<head>
    <script src="https://cdn.mathjax.org/mathjax/latest/MathJax.js?config=TeX-AMS-MML_HTMLorMML" type="text/javascript"></script>
    <script type="text/x-mathjax-config">
        MathJax.Hub.Config({
            tex2jax: {
            skipTags: ['script', 'noscript', 'style', 'textarea', 'pre'],
            inlineMath: [['$','$']]
            }
        });
    </script>
</head>

# grammar – moses0.1
### GRAMMAR OF A STATEMENT

 - ***statement*** -> ***compound-statement***
 | `if-statement`
 | `while-statement`
 | `break-statement`
 | `continue-statement`
 | `return-statement`
 | `expression-statement`
 | `declaration-statement`

 - ***if-statement*** -> 
`if` ***expression*** ***compound-statement***  `else` ***compound-statement***
 
 - ***while-statement*** -> while ***expression*** ***compound-statement***

 - ***break-statement*** -> break ;

 - ***compound-statement*** -> { ***statement*** }

 - ***continue-statement*** -> continue ;

 - ***return-statement*** -> return ***expression?*** ; 
 | `return` ***anonymous-initial?*** `;`

  - ***expression-statement*** -> ***expression?*** ;

### GRAMMAR OF EXPRESSION

 - ***expression*** -> ***assignment-expression***

 - ***assignment-expression*** -> ***condition-or-expression*** 
`|` ***unary-expression***  ***assignment-operator***  ***condition-or-expression***

 - ***assignment-operator*** -> = 
| `*=` 
| `/=` 
| `+=` 
| `-=` 
| `&&=` 
| `||=`
	
 - ***cond-or-expression*** -> ***condition-and-expression***
		| condition-or-expression “||” cond-andition-expression

 - ***cond-and-expression*** -> ***equality-expression***
| ***condition-and-expression*** `&&` ***equality-expression***

 - ***equality-expression*** -> ***rel-expression***
| ***equality-expression*** `==` ***rel-expression***
| ***equality-expression*** `!=` ***rel-expression***

 - ***rel-expression*** -> ***additive-expression***
| ***rel-expression*** `<` ***additive-expression***
| ***rel-expression*** `<=` ***additive-expression***
| ***rel-expression*** `>` ***additive-expression***
| ***rel-expression*** `>=` ***additive-expression***

 - ***additive-expression*** -> ***m-d-expression***
| ***additive-expression*** `+` ***m-d-expression***
| ***additive-expression*** `-` ***m-d-expression***

 - ***multiplicate-expression*** -> ***u-expression***
| ***multiplicate-expression*** `*` ***u-expression***
| ***multiplicate-expression*** `/` ***u-expression***

 - ***u-expression*** -> `-` ***u-expression***
| `!` ***u-expression***
| `++` ***u-expression***
| `--` ***u-expression***
| ***post-expression***

 - ***postfix-expression*** -> ***primary-expression***
| ***post-expression*** `.` `identifier`
| ***post-expression*** `++`
| ***post-expression*** `--`

 - ***primary-expression*** -> `identifier` ***arg-list?***
| `(` ***expression*** `)`
| `INTLITERAL`
| `BOOLLITERAL`

### GRAMMAR OF PARAMETERS

 - ***para-list*** -> `(` ***proper-para-list?*** `)`
 
 - ***proper-para-list*** -> ***para-declaration*** ( ***para-declaration*** )*

 - ***para-declaration*** -> `const`? `identifier` ***type-annotation***

 - ***arg-list*** -> `(` ***proper-arg-list?*** `)`

 - ***proper-arg-list*** -> ***arg***  ( `,` ***arg*** )*

 - ***arg*** -> ***expression*** 
| ***anonymous -initializer***

### GRAMMAR OF DECLARATION

 - ***declaration-statement*** -> ***constant-declaration***
| ***variable-declaration***
| ***class-declaration***
| ***unpack-declaration***

 - ***function-definition*** -> `func` `identifier` ***para-list*** `->` ***return-type*** ***compound-statement***

 - ***return-type*** -> ***type*** 
| `void`
| `anonymous`

 - ***variable-declaration*** -> `var` `identifier` ***initializer*** `;`
| `var` `identifier` ***type-annotation*** `;`

 - ***unpack-declaration*** -> `var` `{` ***unpack-decl-internal*** `}` = ***upack-initial***

 - ***unpack-initial*** -> `identifier` ***arg-list?***

 - ***unpack-decl-internal*** -> `identifier` ( `,` `identifier` )*

 - ***initializer*** -> `=` ***expression***
 | `=` ***anonymous-initializer***

 - ***anonymous -initializer*** -> `{` ***anonymous -initial-internal*** `}`

 - ***anonymous-initial-internal*** -> ***anonymous -initial-element*** ( `,` ***class-initial-element*** )*

 - ***anonymous -initial-element*** -> ***expression*** 
 | ***anonymous -initializer***

 - ***class-declaration*** -> `class` `identifier` ***class-body*** `;`

 - ***class-body*** -> `{` ( ***declaration-statement***
 | ***function-definition*** )* `}`

 - ***constant-declaration*** -> `const` `identifier` ***init-expression*** `;`
 | `const` `identifier` ***type-annotation*** `;`

 - ***init-expression*** -> `=` ***expression***

 - ***type-annotation*** -> `:` ***type***

 - ***anonymous*** -> `{` ***anonymous-internal*** `}`

 - ***anonymous-interal*** -> ***anonymous-type*** ( `,`  ***anonymous-type*** )*

 - ***anonymous-type*** -> `int` 
 | `bool` 
 | ***anonymous***

### GRAMMAR OF PRIMITIVE TYPES
 ***type*** -> `int` 
 | `bool` 
 | `identifier` 
 | ***anonymous***

### GRAMMAR OF IDENTIFIERS

 - `identifier` -> ID

### TOP-LEVEL

 - ***top_level*** : ( ***statement*** | ***function-definition*** )*
 

# moses 0.1 – LL(1)

I convert moses-IR into *LL(1)* form through [HackingOff][1].

> Just a little background
> In formal language theory, an LL grammar is a formal grammar that can be parsed by an LL parser, which parses the input from Left to right, and constructs a Leftmost derivation of the sentence (hence LL, compared with LR parser that constructs a rightmost derivation). A language that has an LL grammar is known as an LL language.
>
> LL parsers are table-based parsers, similar to LR parsers. LL grammars can alternatively be characterized as precisely those that can be parsed by a predictive parser – a [recursive descent parser][3] without backtracking – and these can be readily written by hand. - [LL grammar][2]

 - ***statement*** -> ***compound-statement*** 
 | ***if-statement*** 
 | ***while-statement*** 
 | ***break-statement*** 
 | ***continue-statement*** 
 | ***return-statement*** 
 | ***expression-statement*** 
 | ***declaration-statement***

 - ***if-statement*** -> `if` ***expression*** ***compound-statement*** `else` ***compound-statement***

 - ***while-statement*** -> `while` ***expression*** ***compound-statement***

 - ***break-statement*** -> `break` `;`

 - ***compound-statement*** -> `{` ***statement-list*** `}`

 - ***statement-list*** -> $\epsilon$ 
 | ***statement*** ***statement-list***

 - ***continue-statement*** -> `continue` `;`

 - ***return-statement*** -> `return` ***expression*** `;`

 - ***return-statement*** -> `return` ***anonymous-initial*** `;`

 - ***return-statement*** -> `return` `;`

 - ***expression-statement*** -> ***expression-list*** `;`

 - ***expression-list*** -> ***expression*** 
 | $\epsilon$

 - ***expression*** -> ***assignment-expression***

 - ***assignment-expression*** -> ***condition-or-expression***
| ***u-expression*** ***assignment-operator*** ***condition-expression***

 - ***assignment-operator*** -> `=` 
 | `*=` 
 | `/=` 
 | `+=` 
 | `-=` 
 | `&&=` 
 | `XX=`

 - ***condition-or-expression*** -> ***condition-and-expression*** ***condition-or-expression-tail***

 - ***condition-or-expression-tail*** -> $\epsilon$ 
 | `XX` ***condition-and-expression*** ***condition-or-expression-tail***

 - ***condition-and-expression*** -> ***equality-expression*** ***condition-and-expression-tail***

 - ***condition-and-expression-tail*** -> `&&` ***equality-expression*** `equality-expression-tail` 
 | $\epsilon$

 - ***equality-expression*** -> ***rel-expression*** ***equality-expression-tail***

 - ***equality-expression-tail*** -> $\epsilon$ 
 | `==` ***rel-expression*** ***equality-expression-tail*** 
 | `!=` ***rel-expression*** ***equality-expression-tail***

 - ***rel-expression*** -> ***additive-expression*** ***rel-expression-tail***

 - ***rel-expression-tail*** -> $\epsilon$ 
 | `<` ***additive-expression*** ***rel-expression-tail*** 
 | `<=` ***additive-expression*** ***rel-expression-tail*** 
 | `>` ***additive-expression*** ***rel-expression-tail*** 
 | `>=` ***additive-expression*** ***rel-expression-tail***

 - ***additive-expression*** -> ***m-d-expression*** ***additive-expression-tail***

 - ***additive-expression-tail*** -> $\epsilon$ 
 | `+` ***m-d-expression*** ***additive-expression-tail*** 
 | `-` ***m-d-expression*** ***additive-expression-tail***

 - ***m-d-expression*** -> ***u-expression*** ***m-d-expression-tail***

 - ***m-d-expression-tail*** -> $\epsilon$ 
 | `*` ***u-expression*** ***m-d-expression-tail*** 
 | `/` ***u-expression*** ***m-d-expression-tail***

 - ***u-expression*** -> `-` ***u-expression*** 
 | `!` ***u-expression*** 
 | `++` ***u-expression*** 
 | `--` ***u-expression***
 | ***post-expression***

 - ***post-expression*** -> ***primary-expression*** 
 | ***primary-expression*** ***post-expression-tail***

 - ***post-expression-tail*** -> `.` `identifier` ***post-expression-tail*** 
 | `++` ***post-expression-tail ***
 | `--` ***post-expression-tail*** 
 | $\epsilon$

 - ***primary-expression*** -> `identifier` 
 | `identifier` ***arg-list*** 
 | `(` ***expression*** `)` 
 | ***INT-LITERAL*** 
 | ***BOOL-LITERAL***

 - ***para-list*** -> `(` `)` 
 | `(` ***proper-para-list*** `)`

 - ***proper-para-list*** -> ***para-declaration*** ***proper-para-list-tail***

 - ***proper-para-list-tail*** -> `,` ***para-declaration*** ***proper-para-list-tail*** 
 | $\epsilon$

 - ***para-declaration*** -> `const` `identifier` ***type-annotation*** 
 | `identifier` ***type-annotation***

 - ***arg-list*** -> `(` `)` 
 | `(` ***proper-arg-list*** `)`

 - ***proper-arg-list*** -> ***arg*** ***proper-arg-list-tail***

 - ***proper-arg-list-tail*** -> `,` ***arg*** ***proper-arg-list-tail*** 
 | $\epsilon$

 - ***arg*** -> ***expression*** 
 | ***anonymous -initial***

 - ***declaration-statement*** -> ***constant-declaration*** 
 | ***variable-declaration*** 
 | ***class-declaration*** 
 | ***unpack-declaration***

 - ***function-definition*** -> `func` `identifier` ***para-list*** `->` ***return-type*** ***compound-statement***

 - ***return-type*** -> ***type*** 
 | `void`

 - ***variable-declaration*** -> `var` `identifier` ***initial*** `;` 
 | `var` `identifier` ***type-annotation*** `;`

 - ***unpack-declaration*** -> `var` ***unpack-decls*** `=` ***unpack-initial*** `;`

 - ***unpack-initial*** -> `identifier` 
 | `identifier` ***arg-list***

 - ***unpack-decls*** -> `{` ***unpack-decl-internal*** `}`

 - ***unpack-decl-internal*** -> ***unpack-element*** ***unpack-decl-internal-tail***

 - ***unpack-decl-internal-tail*** -> `,` ***unpack-element*** ***unpack-decl-internal-tail*** 
 | $\epsilon$

 - ***unpack-element*** -> `identifier` 
 | ***unpack-decls***

 - ***class-declaration*** -> ***class*** `identifier` ***class-body*** `;`

 - ***class-body*** -> `{` ***class-member*** `}`

 - ***class-member*** -> ***declaration-statement*** ***class-member*** 
 | ***function-definition*** ***class-member*** 
 | $\epsilon$

 - ***constant-declaration*** -> `const` `identifier` ***init-expression*** `;` 
 | `const` `identifier` ***type-annotation*** `;`

 - ***initial*** -> `=` ***expression*** 
 | `=` ***anonymous-initial***

 - ***anonymous-initial*** -> `{` ***anonymous-initial-internal*** `}`

 - ***anonymous-initial-internal*** -> ***anonymous-initial-element*** ***anonymous-initial-internal-tail***

 - ***anonymous-initial-internal-tail*** -> `,` ***anonymous-initial-element*** ***anonymous-initial-internal-tail*** 
 | $\epsilon$

 - ***anonymous-initial-element*** -> ***expression*** 
 | ***anonymous-initial***

 - ***type-annotation*** -> `:` ***type***

 - ***anonymous*** -> `{` ***anonymous-annotation-internal*** `}`

 - ***anonymous-internal*** -> ***anonymous-type*** ***anonymous-internal-tail***

 - ***anonymous-internal-tail*** -> `,` ***anonymous-type*** ***anonymous-internal-tail*** 
 | $\epsilon$

 - ***anonymous-type*** -> `int` 
 | `bool` 
 | `anonymous`

 - ***type*** -> `int` 
 | `bool` 
 | `identifier` 
 | `anonymous`

 - ***top-level*** -> ***statement*** ***top-level*** 
 | ***function-definition*** ***top-level*** 
 | $\epsilon$

*Note：Since operator `||` will be recognized as a separator, use `XX` instead*

[1]: http://hackingoff.com/
[2]: https://en.wikipedia.org/wiki/LL_grammar
[3]: https://en.wikipedia.org/wiki/Recursive_descent_parser