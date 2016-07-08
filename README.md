# screen
The goal of this project are to create a LLVM pass which runs over the S2N codebase on each commit:

Current S2N defenses:
- constant time routines to maintain speed
- if any errors in client<->server connection: randomization [10,30]
- human review for memory, want: taint analysis to verify code paths are constant, memory access patterns side channels

Purpose of this tool, using annotations
1) A-B no branches
2) A-C no branches that depend on secret memory
3) A-D branch that depend on secret memory: print upper bound of set 
	* proof that no new memory access patterns

Targeting: arm, x86

 
# paper survey based on AWS papers
- writeups in drive

"Hey, You, Get Off of My Cloud: Exploring Information Leakage in Third-Party Compute Clouds", Thomas Ristenpart, Eran Tromer, Hovav Shacham, and Stefan Savage

"Whispers in the Hyper-space: High-speed Covert Channel Attacks in the Cloud", Zhenyu Wu, Zhang Xu, and Haining Wang, The College of William and Mary

"Cross-VM side channels and their use to extract private keys", Y. Zhang, A. Juels, M. K. Reiter, and T. Ristenpart.

"Wait a Minute! A fast, Cross-VM Attack on AES" Gorka Irazoqui, Mehmet Sinan Inci, Thomas Eisenbarth, Berk Sunar (Worcester Polytechnic Institute)

"S$A: A Shared Cache Attack that Works Across Cores and Defies VM Sandboxing-and its Application to AES" Gorka Irazoqui, Thomas Eisenbarth, Berk Sunar

"A Placement Vulnerability Study in Multi-Tenant Public Clouds", Venkatanathan Varadarajan, University of Wisconsin-Madison; Yinqian Zhang, The Ohio State University; Thomas Ristenpart, Cornell Tech; Michael Swift, University of Wisconsin-Madison

"A Measurement Study on Co-residence Threat inside the Cloud", Zhang Xu, College of William and Mary; Haining Wang, University of Delaware; Zhenyu Wu, NEC Laboratories America

"Seriously, get off my cloud! Cross-VM RSA Key Recovery in a Public Cloud", Mehmet Sinan Inci, Berk Gulmezoglu, Gorka Irazoqui, Thomas Eisenbarth, Berk Sunar, Worcester Polytechnic Institute, MA, USA
