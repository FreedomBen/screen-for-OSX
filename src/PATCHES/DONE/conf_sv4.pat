--- 1.1	1992/01/31 19:00:52
+++ config/config.svr4	1992/01/31 19:01:18
@@ -21,6 +21,7 @@
 #define UTHOST
 #undef USRLIMIT
 #define BUGGYGETLOGIN		/* special SVR4 entry */
+#define SHADOWPW
 #undef LOCKPTY
 #define NOREUID
 #undef LOADAV_3DOUBLES
