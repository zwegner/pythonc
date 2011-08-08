node *builtin_print(context *ctx, node *args)
{
    node *s = args->__getitem__(new int_const(0));
    if (!s->is_string())
        s = s->__str__();
    printf("%s\n", s->string_value().c_str());
}

node *builtin_range(context *ctx, node *args)
{
    list *new_list = new list();
    int64_t st;
    int64_t end = args->__getitem__(new int_const(0))->int_value();

    for (st = 0; st < end; st++)
        new_list->append(new int_const(st));
    return new_list;
}
