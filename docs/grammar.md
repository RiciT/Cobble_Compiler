$$
\begin{align}
[\text{Prog}] &\to [\text{Stmt}]^* 
\\
[\text{Stmt}] &\to 
\begin{cases} 
    exit([\text{Expr}]);\\
    def \space \text{ident} = [\text{Expr}]\\
    if([\text{Scope}])[\text{Scope}]
    [\text{Scope}]
\end{cases}
\\
[\text{Scope}] &\to \{[\text{Stmt}^*]\}
\\
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