$$
\begin{align}
[\text{Prog}] &\to [\text{Stmt}]^* 
\\
[\text{Stmt}] &\to 
\begin{cases} 
    def \space [\text{Type}] \space \text{ident} \space [= [\text{Expr}]]^?;\\
    func \space [\text{Type}] \space \text{ident}([\text{ident}]^*)[\text{Scope}]\\
    \text{ident} = [\text{Expr}];\\
    \text{ident}([\text{Expr}]^*);\\
    \text{return} \space [\text{Expr}]^?;\\
    [\text{Scope}]\\
    if([\text{Expr}])[\text{Scope}][\text{IfPred}]\\
    while([\text{Expr}])[\text{Scope}]\\
    print(\text{[Expr]});\\ 
    exit([\text{Expr}]);\\
\end{cases}
\\
[\text{Type}] &\to 
\begin{cases}
void\\
int \\
\epsilon \space \text{(for now, remove later)}
\end{cases}
\\
[\text{Scope}] &\to \{[\text{Stmt}^*]\}
\\
[\text{IfPred}] &\to \begin{cases}
elseif([\text{Expr}])[\text{Scope}][\text{IfPred}] \\
else[\text{Scope}]\\
\epsilon
\end{cases}\\
[\text{Expr}] &\to
\begin{cases}
    \text{[Atom]}\\
    [\text{BinExpr}]
\end{cases}
\\
[\text{BinExpr}] &\to \begin{cases}
[\text{Expr}] * [\text{Expr}] & \text{prec} = 1\\
[\text{Expr}] / [\text{Expr}] & \text{prec} = 1\\
[\text{Expr}] - [\text{Expr}] & \text{prec} = 0\\
[\text{Expr}] + [\text{Expr}] & \text{prec} = 0
\end{cases} \\
[\text{Atom}] &\to \begin{cases}
\text{int\_lit}\\
\text{ident}\\
\text{([Expr])}
\end{cases}
\end{align}
$$

Next steps 
    implement: ==, !=, >=, <= for ints
    implement: while loop