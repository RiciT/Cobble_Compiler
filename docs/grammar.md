$$
\begin{align}
[\text{Prog}] &\to [\text{Stmt}]^* 
\\
[\text{Stmt}] &\to 
\begin{cases} 
    exit([\text{Expr}]);\\
    def \space \text{ident} = [\text{Expr}]\\
    \text{ident} = [\text{Expr}];\\
    if([\text{Expr}])[\text{Scope}][\text{IfPred}]\\
    [\text{Scope}]\\
    while([\text{Expr}])[\text{Scope}]\\
    print(\text{[Expr]});
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