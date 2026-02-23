


src =<<__eos__
            ExprIdent* exprident;
            mcfloat_t exprlitnumber;
            bool exprlitbool;
            ExprLiteralArray exprlitarray;
            ExprImportStmt exprimportstmt;
            ExprLiteralString exprlitstring;
            ExprLiteralMap exprlitmap;
            ExprPrefix exprprefix;
            ExprCall exprcall;
            ExprIfStmt exprifstmt;
            ExprWhileStmt exprwhileloopstmt;
            ExprCodeBlock* exprblockstmt;
            ExprRecover exprrecoverstmt;
            ExprInfix exprinfix;
            ExprLiteralFunction exprlitfunction;
            ExprIndex exprindex;
            ExprAssign exprassign;
            ExprLogical exprlogical;
            ExprTernary exprternary;
            ExprDefine exprdefine;
            ExprForeachStmt exprforeachloopstmt;
            ExprLoopStmt exprforloopstmt;
            AstExpression* exprreturnvalue;
            AstExpression* exprexpression;
__eos__


begin
  src.strip.split(/;/).each do |line|
    line.strip!
    m = line.match(/^(?<typename>\w+(\*)?)\s+(?<varname>\w+)$/)
    if !m then
      $stderr.printf("failed to parse %p\n", line)
    end
    typ = m["typename"]
    var = m["varname"]
    if typ.match?(/^Expr/) then
      typ = "AstExpression::"+typ
    end
    printf("PRINTSIZE<%s>(%p, %p);\n", typ, typ, var);

  end
end

