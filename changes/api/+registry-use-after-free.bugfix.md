**xkbregistry**: fixed use-after-free when a child object (model, layout,
option, ISO 639/3166 code) outlived its context: holding a reference on any
object now keeps the whole context alive until it is released. Objects are
freed together with their context, never individually.
