mofcomp: padenum.bmf

padenum.bmf: padenum.mof
        mofcomp -B:padenum.bmf padenum.mof
        wmimofck padenum.bmf




