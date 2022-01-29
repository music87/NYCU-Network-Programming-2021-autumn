Project 2

> server 1 bug:
> 用 numbered pipe 傳輸很多次大檔案會卡住（測資 4 錯掉）
>
> server 3 bug: 
> client 沒有 attach address
> 測資 8 跟測資 9 錯掉，都會莫名多一個 no name user logout 的廣播，server 本身沒什麼問題（助教也這樣說）
> 我猜會不會是雖然每個 client 下的 command 都會間隔，但是沒有管到每個 client 下 command 與登入登出的順序導致了 race condition
> 看來還是要用 semaphore 來鎖一下 shared memory 的資料...



Project 4

> 1. proxy: 沒有實作到 socks4a (socks4a 跟 socks4 是不同的東西)
> 2. 助教還說我的程式跑的特慢, 會不會是因為 buffer 開得不夠大？