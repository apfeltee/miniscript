


src =<<__eos__
    mcastliteralstring_t exprlitstring;
    mcastliteralarray_t exprlitarray;
    mcastliteralmap_t exprlitmap;
    mcastexprprefix_t exprprefix;
    mcastexprinfix_t exprinfix;
    mcastliteralfunction_t exprlitfunction;
    mcastexprcall_t exprcall;
    mcastexprindex_t exprindex;
    mcastexprassign_t exprassign;
    mcastexprlogical_t exprlogical;
    mcastexprternary_t exprternary;
    mcastexprdefine_t exprdefine;
    mcastexprstmtif_t exprifstmt;
    mcastexpression_t* exprreturnvalue;
    mcastexpression_t* exprexpression;
    mcastexprstmtwhile_t exprwhileloopstmt;
    mcastexprstmtforeach_t exprforeachloopstmt;
    mcastexprstmtforloop_t exprforloopstmt;
    mcastcodeblock_t* exprblockstmt;
    mcastexprstmtimport_t exprimportstmt;
    mcastexprstmtrecover_t exprrecoverstmt;
__eos__



sizesrc =<<__eos__
80      mcgenericdict_t
96      mcvaldict_t
48      mcbasicarray_t
40      mcptrlist_t
3       mcprintconfig_t
32      mcprinter_t
160     mcerror_t
16      mctraceback_t
32      mcastcompiledfile_t
64      mcastexpression_t
32      mccompiledprogram_t
5288    mcstate_t
4168    mcgcmemory_t
24      mcglobalstore_t
96      mcobjdata_t
648     mcerrlist_t
1296    mcastparser_t
6       mcconfig_t
56      mcastsymtable_t
96      mcastcompiler_t
32      mcastsymbol_t
16      mcvalue_t
8       mcvalcmpresult_t
16      mcastlocation_t
40      mcasttoken_t
16      mcastcodeblock_t
16      mcastliteralmap_t
8       mcastliteralarray_t
16      mcastliteralstring_t
16      mcastexprprefix_t
24      mcastexprinfix_t
24      mcastifcase_t
24      mcastliteralfunction_t
16      mcastexprcall_t
24      mcastexprindex_t
24      mcastexprassign_t
24      mcastexprlogical_t
24      mcastexprternary_t
32      mcastident_t
16      mcastfuncparam_t
24      mcastexprdefine_t
16      mcastexprstmtif_t
16      mcastexprstmtwhile_t
24      mcastexprstmtforeach_t
32      mcastexprstmtforloop_t
8       mcastexprstmtimport_t
16      mcastexprstmtrecover_t
64      mcobjfuncscript_t
48      mcobjfuncnative_t
24      mcobjuserdata_t
16      mcobjerror_t
16      mcobjstring_t
24      mcopdefinition_t
24      mcastscopeblock_t
40      mcastscopefile_t
56      mcastscopecomp_t
1032    mcgcobjdatapool_t
208     mcastlexer_t
20      mcastlexprevinfo_t
80      mcvmframe_t
24      mctraceitem_t
24      module_t
16      mcstoddiyfp_t
8       mcstoddiyfpconv_t
8       mcvalunion_t
64      mcobjunion_t
32      mcexprunion_t
32      mcfuncfvunion_t
8       mcfuncnameunion_t
__eos__

begin
  slines = []
  src.split(/\n/).each do |ln|
    ln.strip!
    next if ln.empty?
    t = ln.match(/\b(?<t>\w+_t)\b/)
    if not t then
      raise 'fucked up parsing src line: '+ln.dump+''
    end
    slines.push({line: ln, type: t['t']})
  end
  sizes = {}
  sizesrc.split(/\n/).each do |ln|
    ln.strip!
    next if ln.empty?
    m = ln.match(/^(?<sz>\d+)\s+(?<t>\w+_t\b)/)
    if not m then
      raise 'fucked up parsing size line '+ln.dump+''
    end
    sizes[m['t']] = m['sz'].to_i
  end

  slines.each do |stuff|
    stuff[:size] = sizes[stuff[:type]]
  end
  slines.sort_by{|stuff| stuff[:size]}.each do |stuff|
    printf("%s\n", stuff[:line])
  end
end

