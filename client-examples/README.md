# client-examples

This directory contains some examples for setting up an environment to send your code to otojsd.

## otojsc - shell/terminal

otojsc is a shell script that makes posting to otojsd via curl accessible with easy-to-remember options.

```
client-examples/otojsc -h localhost -p 14609 examples/otojs-example.js
```

Instead of specifying a port number, using `-f` reads the `.otojsd_port` file output by otojsd's `-f` option to determine the destination.

```
client-examples/otojsc -f examples/otojs-example.js
```

To send code from standard input, specify `-` instead of a filename.

```
pbpaste | client-examples/otojsc -
```

## CotEditor

To send code from CotEditor, copy files into the scripts folder ( ~Library/Application Scripts/com.coteditor.CotEditor )

* Post selection to otojsd (Command+Shift+O)
* Post file to otojsd (Command+Shift+Option+O)

## VSCode - Visual Studio Code

To send code from VSCode, copy and paste these snippets into tasks.json and keybindings.json.

* Post selection to otojsd (Command+Shift+O)
* Post file to otojsd (Command+Shift+Option+O)

## html - Web interface

Launch otojsd with the -d (--document-root) option.

```
release/otojsd -d client-examples/html
```

Open `http://localhost:14609` in your browser to view `client-examples/html/index.html` and send code from the page.
Since otojsd is a kind of minimal HTTP server, you can add custom web interfaces.
