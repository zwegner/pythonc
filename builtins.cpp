node *builtin_print(context *ctx, node *args)
{
    node *s = args->__getitem__(new int_const(0));
    if (!s->is_string())
        s = s->__str__();
    printf("%s\n", s->string_value().c_str());
}
