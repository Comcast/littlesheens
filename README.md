# A minimalist C library for working with Sheens

## Summary

A minimalist [Sheens](https://github.com/Comcast/sheens)
implementation in ECMAScript that's executed by
[Duktape](http://duktape.org/) and wrapped in a C library.  Yes, that
does sound a little odd.

The stripped 32-bit demo executable is 360KB.  The supporting
ECMAScript code (un-minified) is less than 20KB.

See that [Sheens repo](https://github.com/Comcast/sheens) for more
documentation about these machines.

## License

This repo is licensed under [Apache License 2.0](LICENSE).

## API

See `machines.h`.

## Demo

For now, building the demo requires two tools written in
[Go](https://golang.org/), so you need Go installed to build this
demo.  (We'll remove this unncessary dependency soon.)

```Shell
go get github.com/bronze1man/yaml2json
go get github.com/tdewolff/minify/cmd/minify
make demo
```

If that works, you'll have an `mdemo` executable.

```Shell
./mdemo
```

## Another demo

```Shell
make sheens
make specs/turnstile.js

# Define a "crew" of two machines.
cat<<EOF > crew.js
{"id":"simpsons",
 "machines":{
   "doubler":{"spec":"double","node":"listen","bs":{"count":0}},
   "turnstile":{"spec":"turnstile","node":"locked","bs":{}}}}
EOF

# Send messages to that crew.
cat<<EOF | ./sheens
{"double":1}
{"double":10}
{"double":100}
{"input":"push"}
{"input":"coin"}
{"input":"coin"}
{"input":"push"}
EOF
```

The above is in `demo.sh`.


## Code of Conduct

We take our [code of conduct](CODE_OF_CONDUCT.md) seriously. Please
abide by it.


## Contributing

Please read our [contributing guide](CONTRIBUTING.md) for details on
how to contribute to our project.


## References

1. [Sheens](https://github.com/Comcast/sheens)
1. [Duktape](http://duktape.org/)
