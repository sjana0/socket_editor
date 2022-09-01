# socket editor
server connects with multiple clients with select
1.	On connection server sends successful connection message
2.	client processes each and every command with a check_command function to check if the command format is right.
3.	server processes each command by a relay of messages of sending actual information and acknowledgement messages with client.
	In the client side client only checks if a acknowledge message is recieved and prints a successful command message.
4.	This also helps server to send error messages during acknowledgement and to send successful command messages.
5.	The server file upload is limited to 1024 lines per line of file.
6.	each files that has been uploaded to the server is kept at a designated folder called server_files,
	server program at each initiation checks if the server_files directory exists or not and keeps each file with the same name in that directory.
7.	for multiple clients to send invites a child process waits for message from server and keeps on asking for input from client on invite answer until a proper "yes/no".
8.	for client ids server side keeps an array of string "client_ids" to keep ids saved,
	each id is generated 5 digit long random number string.
9.	each file uploaded server keeps an extra file with all the information about the file with the name suffixed with "perm_".
	informations are in order filename, no of lines, owner, list of collaborators with permission.
10.	For invitation each invitation server sends a request to the respective client at the moment any invitation comes to the server.
	server then waits for the respective client to send a definitive answer then server change the permission of the file and sends results to the owner.
11.	on upload server creates a permission file(containing filename, no of lines, and collaborators) and edits a user file named with "client_ids" to store the file names the user will upload.

For all other commands:
1.	/users: View all active clients lists with numbered according to the server
2.	/files: View all ﬁles that have been uploaded to the server, along with all
	details (owners, collaborators, permissions and the no. of lines in the ﬁle).
3.	/upload <filename>: Upload the local ﬁle ﬁlename to the server
4.	/download <filename>: Download the server ﬁle ﬁlename to the client, if
	given client has permission to access that ﬁle
5.	/invite <filename> <client_id> <permission>: Invite client client_id
	to join as collaborator for ﬁle ﬁlename (should already be present on server).
	permission can be either of V/E signifying viewer/editor.
6.	/read <filename> <start_idx> <end_idx>: Read from ﬁle ﬁlename
	starting from line index start_idx to end_idx . If only one index is speciﬁed, read
	that line. If none are speciﬁed, read the entire ﬁle.
7.	/insert <filename> <idx> “<message>”: Write message at the line
	number speciﬁed by idx into ﬁle ﬁlename . If idx is not speciﬁed, insert at the end.
	Quotes around the message to demarcate it from the other ﬁelds.
8.	/delete <filename> <start_idx> <end_idx>: Delete lines starting
	from line index start_idx to end_idx from ﬁle ﬁlename. If only one index is
	speciﬁed, delete that line. If none are speciﬁed, delete the entire contents of the
	ﬁle.
9.	/exit: Disconnects from the server, and then all ﬁles which this client owned
	should be deleted at the server, and update the permission ﬁle.

	run server file with "g++ server.cpp -o server && ./server"
	run the client using "g++ client.cpp -o client && ./client"