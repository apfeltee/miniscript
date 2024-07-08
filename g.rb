
src=<<__eos__

typedef enum arcane_error_type arcane_error_type_t;
typedef enum arcane_object_type arcane_object_type_t;
typedef enum token_type_t token_type_t;
typedef enum operator_t operator_t;
typedef enum expression_type expression_type_t;
typedef enum statement_type statement_type_t;
typedef enum object_type_t object_type_t;
typedef enum opcode_val opcode_val_t;
typedef enum symbol_type symbol_type_t;
typedef enum error_type error_type_t;
typedef enum precedence precedence_t;


__eos__

begin
  dt = []
  src.split(/;/).each do |ln|
    ln.strip!
    next if ln.empty?
    b = ln.split(/\s+/)
    stname = b[2]
    tdname = b[3]
    if stname != tdname then
      $stderr.printf("must be renamed: struct %s --> %s\n", stname, tdname)
      dt.push(stname)
    end
  end
  print('\b(' + dt.join('|') + ')\b')
end

