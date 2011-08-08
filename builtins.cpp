node *builtin_len(context *ctx, node *args)
{
    return args->__getitem__(new int_const(0))->__len__();
}

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

node *builtin_set(context *ctx, node *args)
{
    return new set();
}

node *builtin_open(context *ctx, node *args)
{
    node *path = args->__getitem__(new int_const(0));
    node *mode = args->__getitem__(new int_const(1));
    if (!path->is_string() || !mode->is_string())
        error("bad arguments to open()");
    file *f = new file(path->string_value().c_str(), mode->string_value().c_str());
    return f;
}

node *builtin_fread(context *ctx, node *args)
{
    node *f = args->__getitem__(new int_const(0));
    node *len = args->__getitem__(new int_const(1));
    if (!f->is_file() || !len->is_int_const())
        error("bad arguments to fread()");
    return ((file *)f)->read(len->int_value());
}
